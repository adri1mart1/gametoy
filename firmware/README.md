# Gametoy

This project is about a game that I have built for Christmas 2024, using an STM32 dev kit board and Zephyr RTOS.

Find more information on its dedicated [github page](https://adri1mart1.github.io/)

## Build

Install the [Zephyr SDK](https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html) first.

Create a Python virtual environment and install west:

```bat
cd firmware
py -m venv venv
.\venv\Scripts\activate
pip install west
```

Initialize the west workspace and download Zephyr + HAL modules (~4 GB):

```bat
west init -l .
west update
```

Install Zephyr Python dependencies:

```bat
pip install -r ..\zephyr\scripts\requirements.txt
```

Build:

```bat
set ZEPHYR_SDK_INSTALL_DIR=C:\Users\<you>\zephyr-sdk-0.16.5
west build -b stm32f0_disco --shield stm32f0_gtp app
```

Flash:

```bat
west flash
```

## License

This code is licensed under the Apache 2.0.
