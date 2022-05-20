# Arduino Print

Methods used to support Arduino Print functionality.

### setCursor()

This method is called set the "cursor" position in the device. The library supports the Arduino `Print` interface, enabling the use of a `print()` and `println()` methods. The set cursor position defines where to start text output for this functionality.

```c++
void setCursor(uint8_t x, uint8_t y)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| x | `uint8_t` | The X coordinate of the cursor|
| y | `uint8_t` | The Y coordinate of the cursor|

### setColor()

This method is called to set the current color of the system. This is used by the Arduino `Print` interface functionality


```c++
void setColor(uint8_t clr)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `clr` | `uint8_t` | The color to set. 0 = black, > 0 = white|

### getColor()

This method is called to get the current color of the system. This is used by the Arduino `Print` interface functionality


```c++
uint8_t getColor(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| return value| `uint8_t` | The current color|
