// ///////////////////////// Package Header Files ////////////////////////////
#include "neuton_result_postprocessing.h"
// ////////////////////// Package Group Header Files /////////////////////////
// /////////////////// Application Global Header Files ///////////////////////
// /////////////////// 3rd Party Software Header Files ///////////////////////
#include <sl_assert.h>
// ////////////////////// Standard C++ Header Files //////////////////////////
// /////////////////////// Standard C Header Files ///////////////////////////
#include <string.h>

///
#define PREVIOUS_PREDICTION_NUM                 (4)
///

//////////////////////////////////////////////////////////////////////////////

typedef struct prediction_ctx_s
{
    /** Prediction target */
    uint16_t target;

    /** Prediction probability */
    float probability;
} prediction_ctx_t;

typedef struct prediction_tracer_s
{
    /** Current prediction index */
    int index;

    /** Current prediction target */
    uint16_t target;

    /** Previous predictions context */
    prediction_ctx_t prev[PREVIOUS_PREDICTION_NUM];
} prediction_tracer_t;

/**
 * @brief Class prediction conditions for postprocessing
 * 
 */
typedef struct class_prediction_condition_s
{
    /** Minimum number of repetitions of a class for prediction */
    uint16_t min_repeat_count;

    /** Minimum probability threshold for prediction */
    float probablitity_threshold;
} class_prediction_condition_t;

//////////////////////////////////////////////////////////////////////////////

static const class_prediction_condition_t* get_class_condition_(uint8_t predicted_target);
static const char* get_name_by_target_(uint8_t predicted_target);

//////////////////////////////////////////////////////////////////////////////

void neuton_result_postprocess(const uint16_t predicted_target, const float prob,
                               neuton_result_rdy_cb_t callback)
{
    static prediction_tracer_t tracer_ = {0U};

    uint16_t target = predicted_target;
    float probability = prob;

    if ((target == NEUTON_LABEL_UNKNOWN) || (target == NEUTON_LABEL_IDLE))
    {
        /** Reset tracer for UNKNOWN and IDLE classes */
        tracer_.index = 0;
        tracer_.target = target;
    }
    else
    {
        /** Reset tracer if predicted class is not the same as previous */
        if (tracer_.target != target)
        {
            tracer_.index = 0;
        } else
        {
            if (tracer_.index > PREVIOUS_PREDICTION_NUM)
            {
                for (int i = 0; i < PREVIOUS_PREDICTION_NUM - 1; i++)
                {
                    tracer_.prev[i].probability = tracer_.prev[i + 1].probability;
                    tracer_.prev[i].target = tracer_.prev[i + 1].target;
                }
                tracer_.index--;
            } else
            {
                tracer_.prev[tracer_.index].probability = probability;
                tracer_.prev[tracer_.index].target = target;

                tracer_.index++;
            }
        }

        tracer_.target = target;

        const class_prediction_condition_t* class_condition = get_class_condition_(target);
        EFM_ASSERT((class_condition != NULL));

        /** Сlass is labled as NEUTON_LABEL_UNKNOWN if the number of repetitions does not exceed the threshold */
        if (tracer_.index >= class_condition->min_repeat_count)
        {
            /** Calculate average probability for last N predictions of the same class */
            float average_prob = 0.0f;

            for (int i = 0; i < tracer_.index; ++i)
                average_prob += tracer_.prev[i].probability;

            average_prob = average_prob / tracer_.index;

            /** If average probability is less the class probability threshold,
             * the class is labled as NEUTON_LABEL_UNKNOWN */
            if (average_prob < class_condition->probablitity_threshold)
                target = NEUTON_LABEL_UNKNOWN;
            else
                probability = average_prob;
        }
        else
        {
            target = NEUTON_LABEL_UNKNOWN;
        }
    }

    /** Provide result to user callback */
    if (callback)
        callback(target, probability, get_name_by_target_(target));
}

//////////////////////////////////////////////////////////////////////////////

static const char* get_name_by_target_(uint8_t predicted_target)
{

    static const char* LABEL_VS_NAME[] = 
    {
        [NEUTON_LABEL_IDLE]           = "IDLE",
        [NEUTON_LABEL_UNKNOWN]        = "UNKNOWN",
        [NEUTON_LABEL_SCREWDRIVER]    = "SCREWDRIVER",
        [NEUTON_LABEL_BRUSHING_HAIR]  = "BRUSHING HAIR",
        [NEUTON_LABEL_WAVING]         = "HAND WAVING",
        [NEUTON_LABEL_WASHING_HANDS]  = "WASHING HANDS",
        [NEUTON_LABEL_CLAPPING]       = "CLAPPING",
        [NEUTON_LABEL_WIPING]         = "WIPING"
    };

    static const uint8_t LABELS_CNT = sizeof(LABEL_VS_NAME) / sizeof(LABEL_VS_NAME[0]);

    return (predicted_target < LABELS_CNT) ? LABEL_VS_NAME[predicted_target] : NULL;
}

//////////////////////////////////////////////////////////////////////////////

static const class_prediction_condition_t* get_class_condition_(uint8_t predicted_target)
{
    static const class_prediction_condition_t LABEL_VS_CONFIG[] = 
    {
        [NEUTON_LABEL_IDLE]           = {0, 0.0},
        [NEUTON_LABEL_UNKNOWN]        = {0, 0.0},
        [NEUTON_LABEL_SCREWDRIVER]    = {3, 0.7},
        [NEUTON_LABEL_BRUSHING_HAIR]  = {3, 0.7},
        [NEUTON_LABEL_WAVING]         = {3, 0.7},
        [NEUTON_LABEL_WASHING_HANDS]  = {3, 0.7},
        [NEUTON_LABEL_CLAPPING]       = {3, 0.7},
        [NEUTON_LABEL_WIPING]         = {3, 0.8},
    };

    static const uint8_t LABELS_CNT = sizeof(LABEL_VS_CONFIG) / sizeof(LABEL_VS_CONFIG[0]);

    return (predicted_target < LABELS_CNT) ? &LABEL_VS_CONFIG[predicted_target] : NULL;
}
