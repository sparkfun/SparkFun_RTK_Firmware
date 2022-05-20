# Scrolling

Methods for device scrolling

### scrollStop()
If the device is in a scrolling mode, calling this method stops the scroll, and restores the device to normal display operation. This action is performed immediately.

```c++
void scrollStop(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| NONE|  |  |



### scrollRight()
This method is called to start the device scrolling the displayed graphics to the right. This action is performed immediately.

The screen will scroll until the ```scrollStop()``` method is called.

```c++
void scrollRight(uint8_t start, uint8_t stop, uint8_t interval)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `start` | `uint8_t` | The start page address of the scroll - valid values are 0 thru 7|
| `stop` | `uint8_t` | The stop/end page address of the scroll - valid values are 0 thru 7|
| `interval` | `uint8_t` | The time interval between scroll step - values listed below |

Defined values for the `interval` parameter:

| Defined Symbol | Time Interval Between Steps |
| :--- | :---|
|`SCROLL_INTERVAL_2_FRAMES`   |	2 |
|`SCROLL_INTERVAL_3_FRAMES`   | 3 |
|`SCROLL_INTERVAL_4_FRAMES`   | 4 |
|`SCROLL_INTERVAL_5_FRAMES`   | 5  |
|`SCROLL_INTERVAL_25_FRAMES`  | 25 |
|`SCROLL_INTERVAL_64_FRAMES`  | 64 |
|`SCROLL_INTERVAL_128_FRAMES` | 128 |
|`SCROLL_INTERVAL_256_FRAMES` | 256 |

### scrollVertRight()
This method is called to start the device scrolling the displayed graphics vertically and to the right. This action is performed immediately.

The screen will scroll until the ```scrollStop()``` method is called.

```c++
void scrolVertlRight(uint8_t start, uint8_t stop, uint8_t interval)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `start` | `uint8_t` | The start page address of the scroll - valid values are 0 thru 7|
| `stop` | `uint8_t` | The stop/end page address of the scroll - valid values are 0 thru 7|
| `interval` | `uint8_t` | The time interval between scroll step - values listed in ```scrollRight``` |

### scrollLeft()
This method is called start to the device scrolling the displayed graphics to the left. This action is performed immediately.

The screen will scroll until the ```scrollStop()``` method is called.

```c++
void scrollLeft(uint8_t start, uint8_t stop, uint8_t interval)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `start` | `uint8_t` | The start page address of the scroll - valid values are 0 thru 7|
| `stop` | `uint8_t` | The stop/end page address of the scroll - valid values are 0 thru 7|
| `interval` | `uint8_t` | The time interval between scroll step - values listed in ```scrollRight``` |

### scrollVertLeft()
This method is called to start the device scrolling the displayed graphics vertically and to the left. This action is performed immediately.

The screen will scroll until the ```scrollStop()``` method is called.

```c++
void scrolVertlLeft(uint8_t start, uint8_t stop, uint8_t interval)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `start` | `uint8_t` | The start page address of the scroll - valid values are 0 thru 7|
| `stop` | `uint8_t` | The stop/end page address of the scroll - valid values are 0 thru 7|
| `interval` | `uint8_t` | The time interval between scroll step - values listed in ```scrollRight``` |
