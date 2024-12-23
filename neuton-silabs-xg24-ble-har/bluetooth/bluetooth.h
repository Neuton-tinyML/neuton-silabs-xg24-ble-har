#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"

#ifdef __cplusplus
extern "C" {
#endif
bool sl_bt_send_data_str(const char* data);
bool sl_bt_is_connected(void);
#ifdef __cplusplus
}
#endif
