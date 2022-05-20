# Graphics Methods

Methods used to draw and display graphics.

### display()
When called, any pending display updates are sent to the connected OLED device. This includes drawn graphics and erase commands.

```c++
void display(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| NONE|  |  |

### erase()
Erases all graphics on the device, placing the display in a blank state. The erase update isn't sent to the device until the next ```display()``` call on the device.

```c++
void erase(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| NONE|  |  |

### pixel()

Set the value of a pixel on the screen.

```c++
void pixel(uint8_t x, uint8_t y, uint8_t clr)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| x | `uint8_t` | The X coordinate of the pixel to set|
| y | `uint8_t` | The Y coordinate of the pixel to set|
| clr | `uint8_t` | **optional** The color value to set the pixel. This defaults to white (1).|

### line()

Draw a line on the screen. 

Note: If a line is horizontal (y0 = y1) or vertical (x0 = x1), optimized draw algorithms are used by the library.

```c++
void line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t clr)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| x0 | `uint8_t` | The start X coordinate of the line|
| y0 | `uint8_t` | The start Y coordinate of the line|
| x1 | `uint8_t` | The end X coordinate of the line|
| y1 | `uint8_t` | The end Y coordinate of the line|
| clr | `uint8_t` | **optional** The color value to draw the line. This defaults to white (1).|

### rectangle()

Draw a rectangle on the screen. 

```c++
void rectangle(uint8_t x0, uint8_t y0, uint8_t width, uint8_t height, uint8_t clr)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| x0 | `uint8_t` | The start X coordinate of the rectangle - upper left corner|
| y0 | `uint8_t` | The start Y coordinate of the rectangle - upper left corner|
| width | `uint8_t` | The width of the rectangle|
| height | `uint8_t` | The height of the rectangle|
| clr | `uint8_t` | **optional** The color value to draw the line. This defaults to white (1).|

### rectangleFill()

Draw a filled rectangle on the screen. 

```c++
void rectangleFill(uint8_t x0, uint8_t y0, uint8_t width, uint8_t height, uint8_t clr)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| x0 | `uint8_t` | The start X coordinate of the rectangle - upper left corner|
| y0 | `uint8_t` | The start Y coordinate of the rectangle - upper left corner|
| width | `uint8_t` | The width of the rectangle|
| height | `uint8_t` | The height of the rectangle|
| clr | `uint8_t` | **optional** The color value to draw the line. This defaults to white (1).|

### circle()

Draw a circle on the screen. 

```c++
void circle(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t clr)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| x0 | `uint8_t` | The X coordinate of the circle center|
| y0 | `uint8_t` | The Y coordinate of the circle center|
| radius | `uint8_t` | The radius of the circle|
| clr | `uint8_t` | **optional** The color value to draw the circle. This defaults to white (1).|

### circleFill()

Draw a filled circle on the screen. 

```c++
void circleFill(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t clr)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| x0 | `uint8_t` | The X coordinate of the circle center|
| y0 | `uint8_t` | The Y coordinate of the circle center|
| radius | `uint8_t` | The radius of the circle|
| clr | `uint8_t` | **optional** The color value to draw the circle. This defaults to white (1).|

### bitmap()

Draws a bitmap on the screen.

The bitmap should be 8 bit encoded - each pixel contains 8 y values.

```c++
void bitmap(uint8_t x0, uint8_t y0, uint8_t *pBitmap, uint8_t bmp_width, uint8_t bmp_height )
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| x0 | `uint8_t` | The X coordinate to place the bitmap - upper left corner|
| y0 | `uint8_t` | The Y coordinate to place the bitmap - upper left corner|
| pBitmap | `uint8_t *` | A pointer to the bitmap array|
| bmp_width | `uint8_t` | The width of the bitmap|
| bmp_height | `uint8_t` | The height of the bitmap|

### bitmap()

Draws a bitmap on the screen.

The bitmap should be 8 bit encoded - each pixel contains 8 y values.

The coordinate [x1,y1] allows for only a portion of bitmap to be drawn. 

```c++
void bitmap(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, 
				uint8_t *pBitmap, uint8_t bmp_width, uint8_t bmp_height )
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| x0 | `uint8_t` | The X coordinate to place the bitmap - upper left corner|
| y0 | `uint8_t` | The Y coordinate to place the bitmap - upper left corner|
| x1 | `uint8_t` | The end X coordinate of the bitmap - lower right corner|
| y1 | `uint8_t` | The end Y coordinate of the bitmap - lower right corner|
| pBitmap | `uint8_t *` | A pointer to the bitmap array|
| bmp_width | `uint8_t` | The width of the bitmap|
| bmp_height | `uint8_t` | The height of the bitmap|

### bitmap()

Draws a bitmap on the screen using a Bitmap object for the bitmap data.

```c++
void bitmap(uint8_t x0, uint8_t y0, QwiicBitmap& bitmap)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| x0 | `uint8_t` | The X coordinate to place the bitmap - upper left corner|
| y0 | `uint8_t` | The Y coordinate to place the bitmap - upper left corner|
| Bitmap | `QwiicBitmap` | A bitmap object|

### text()

Draws a string using the current font on the screen.

```c++
void text(uint8_t x0, uint8_t y0, const char * text, uint8_t clr)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| x0 | `uint8_t` | The X coordinate to start drawing the text|
| y0 | `uint8_t` | The Y coordinate to start drawing the text|
| text | `const char*` | The string to draw on the screen |
| text | `String` | The Arduino string to draw on the screen |
| clr | `uint8_t` | **optional** The color value to draw the circle. This defaults to white (1).|
