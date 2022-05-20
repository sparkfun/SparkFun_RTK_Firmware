# Device Operations

Methods to setup the device, get device information and change display options.

## Initialization

### begin()
This method is called to initialize the OLED library and connection to the OLED device. This method must be called before calling any graphics methods. 

```c++
bool begin(TwoWire &wirePort, uint8_t address)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| `wirePort` | `TwoWire` | **optional**. The Wire port. If not provided, the default port is used|
| `address` | `uint8_t` | **optional**. I2C Address. If not provided, the default address is used.|
| return value | `bool` | ```true``` on success, ```false``` on startup failure |

### reset()
When called, this method reset the library state and OLED device to their intial state. Helpful to reset the OLED after waking up a system from a sleep state.

```C++ 
void reset()
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| return value | `bool` | ```true``` on success, ```false``` on startup failure |


## Geometry

### getWidth()
This method returns the width, in pixels, of the connected OLED device

```c++
uint8_t getWidth(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| return value | `uint8_t` | The width in pixels of the connected OLED device |

### getHeight()
This method returns the height, in pixels, of the connected OLED device

```c++
uint8_t getHeight(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| return value | `uint8_t` | The height in pixels of the connected OLED device |

## Display Modes

### invert()
This method inverts the current graphics on the display. This results of this command happen immediatly.

```c++
void invert(bool bInvert)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| ```bInvert``` | `bool` | ```true``` - the screen is inverted. ```false``` - the screen is set to normal |

### flipVertical()
When called, the screen contents are flipped vertically if the flip parameter is true, or restored to normal display if the flip parameter is false. 

```c++
void flipVertical(bool bFlip)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| ```bFlip``` | `bool` | ```true``` - the screen is flipped vertically. ```false``` - the screen is set to normal |

### flipHorizontal()
When called, the screen contents are flipped horizontally if the flip parameter is true, or restored to normal display if the flip parameter is false. 

```c++
void flipHorizontal(bool bFlip)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| ```bFlip``` | `bool` | ```true``` - the screen is flipped horizontally. ```false``` - the screen is set to normal |

### displayPower()
Used to turn the OLED display on or off. 

```c++
void displayPower(bool bEnable)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| ```bEnable``` | `bool` | ```true``` - the OLED display is powered on (default). ```false``` - the OLED dsiplay is powered off. |

