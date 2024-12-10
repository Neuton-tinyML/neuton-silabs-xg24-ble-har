import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib import animation
from matplotlib import style
import matplotlib.font_manager as fm
from PIL import Image
import serial
from serial import Serial

PORT = "/dev/cu.usbmodem0004402992621"

target_codes = {
    "0": "unknown",
    "1": "screwdriver",
    "2": "brushing_hair",
    "3": "waving",
    "4": "washing_hands",
    "5": "clapping",
    "6": "wiping_table",
    "7": "idle",
}

ser = Serial(
    port=PORT,
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0,
)


def handle_serial():
    serial_str = ser.readline().decode("utf-8")
    serial_str = serial_str.strip()
    predicted_class = str(serial_str.split(",")[0])
    probability = serial_str.split(",")[1]
    probability = probability.strip()
    return predicted_class, probability


class HarAnimation:

    def __init__(self):

        self.background = Image.open("../images/background.png")
        self.background = self.background.resize((1600, 1600))

    def add_text(self, ax, text, font_file, font_size, x, y):
        font = fm.FontProperties(fname=font_file)
        ha = "center"
        color = "black"
        ax.text(
            x,
            y,
            text,
            ha=ha,
            color=color,
            size=font_size,
            fontproperties=font,
        )

    def get_current_activity(self):
        """Read predicted class from file"""
        prediction = open("prediction.txt", "r").read().strip()
        predicted_class = int(prediction.split(":")[0])
        probability = float(prediction.split(":")[1])
        return predicted_class, probability

    def show_image(self, ax, image_name):
        """Display an image on the axes"""
        self.background = Image.open("../images/background.png")
        self.background = self.background.resize((2400, 1600))
        sylabs_logo = Image.open("../images/sylabs_logo.png")
        self.background.paste(sylabs_logo, (40, 20), mask=sylabs_logo)
        neuton_logo = Image.open("../images/neuton_logo.png")
        # increase logo by 10%
        neuton_logo = neuton_logo.resize(
            (int(neuton_logo.size[0] * 1.2), int(neuton_logo.size[1] * 1.2))
        )
        self.background.paste(neuton_logo, (2010, 10), mask=neuton_logo)
        image = Image.open(image_name)
        image = image.resize((800, 800))
        self.background.paste(image, (800, 400), mask=image)
        ax.imshow(self.background)

        # Define the outline color and width
        outline_color = "black"
        outline_width = 10

        # Add a rectangle patch as the outline frame
        rect = patches.Rectangle(
            (0, 0),  # (x, y) position of the bottom left corner
            self.background.size[0],  # width
            self.background.size[1],  # height
            linewidth=outline_width,
            edgecolor=outline_color,
            facecolor="none",
            alpha=0.05,
        )
        ax.add_patch(rect)

    def main(self):
        """
        Displays a live animation of the predicted classes.
        Reads the cleaning progress from a file displays predictions on the background.
        Returns:
            None

        """
        fig, ax = plt.subplots(1, figsize=(10, 7))
        ax.axis("off")

        ax.imshow(self.background)

        def animate_teeth(i):
            try:
                predicted_class, probability = handle_serial()
                activity = target_codes[predicted_class]
                ax.clear()
                self.add_text(
                    ax,
                    text="Current prediction",
                    font_file="../fonts/Futura light bt.ttf",
                    font_size=20,
                    x=self.background.size[0] / 2,
                    y=140,
                )
                self.add_text(
                    ax,
                    " ".join(activity.split("_")),
                    font_file="../fonts/Futura heavy font.ttf",
                    font_size=30,
                    x=self.background.size[0] / 2,
                    y=270,
                )
                self.add_text(
                    ax,
                    f"Probability: {probability}%",
                    font_file="../fonts/Futura light bt.ttf",
                    font_size=20,
                    x=2050,
                    y=1550,
                )
                self.show_image(ax, f"../images/{activity}.png")
                ax.axis("off")

            except IndexError:
                pass

        ani_teeth = animation.FuncAnimation(fig, animate_teeth, interval=100)
        plt.show()


if __name__ == "__main__":
    c = HarAnimation()
    c.main()
