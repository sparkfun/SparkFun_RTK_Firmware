# Drawing Settings/State

Methods for setting the drawing state of the library.

### setFont()

This method is called to set the current font in the library. The current font is used when calling the ```text()``` method on this device. 

The default font for the device is `5x7`.

```c++
void setFont(QwiicFont& theFont)
void setFont(const QwiicFont * theFont)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `theFont` | `QwiicFont` | The font to set as current in the device|
| `theFont` | `QwiicFont*` | Pointer to the font to set as current in the device.|

For the library, fonts are added to your program by including them via include files which are part of this library. 

The following fonts are included:

| Font | Include File | Font Variable | Description|
| :--- | :--- | :--- | :--- |
| 5x7 | `<res/qw_fnt_5x7.h>` | `QW_FONT_5X7`| A full, 5 x 7 font|
| 31x48 | `<res/qw_fnt_31x48.h>` |`QW_FONT_31X48`| A full, 31 x 48 font|
| Seven Segment | `<res/qw_fnt_7segment.h>` |`QW_FONT_7SEGMENT`| Numbers only|
| 8x16 | `<res/qw_fnt_8x16.h>` | `QW_FONT_8X16`| A full, 8 x 16 font|
| Large Numbers | `<res/qw_fnt_largenum.h>` |`QW_FONT_LARGENUM`| Numbers only|

For each font, the font variables are objects with the following attributes:

| Attribute | Value |
| :--- | :--- | 
| `width` | The font width in pixels|
| `height` | The font height in pixels|
| `start` | The font start character offset|
| `n_chars` | The number of characters|
| `map_width` | The width of the font map|

Example use of a font object attribute:
```C++
#include <res/qw_fnt_31x48.h>
   
int myFontWidth = QW_FONT_31X48.width;
```

### getFont()
This method returns the current font for the device.

```c++
QwiicFont * getFont(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| return value | `QwiicFont*` | A pointer to the current font. See `setFont()` for font object details.|

### getFontName()

This method returns the height in pixels of a provided String based on the current device font.

```c++
String getFontName(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| return value | String | The name of the current font.|

### getStringWidth()

This method returns the width in pixels of a provided String based on the current device font.

```c++
unsigned int getStringWidth(String text)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| text | `String` | The string used to determine width |
| return value | `unsigned int` | The width of the provide string, as determined using the current font.|

### getStringHeight()

This method returns the height in pixels of a provided String based on the current device font.

```c++
unsigned int getStringHeight(String text)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| text | `String` | The string used to determine height |
| return value | `unsigned int` | The height of the provide string, as determined using the current font.|

### setDrawMode()
This method sets the current draw mode for the library. The draw mode determines how pixels are set on the screen during drawing operations. 

```c++
void setDrawMode(grRasterOp_t rop)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| rop | `grRasterOp_t` | The raster operation (ROP) to set the graphics system to.|

Raster operations device how source (pixels to draw) are represented on the destination device. The available Raster Operation (ROP) codes are:

| ROP Code | Description|
| :--- | :--- |
| grROPCopy | **default** Drawn pixel values are copied to the device screen|
| grROPNotCopy | A not operation is applied to the source value before copying to screen|
| grROPNot | A not operation is applied to the destination (screen) value |
| grROPXOR | A XOR operation is performed between the source and destination values|
| grROPBlack | A value of 0, or black is drawn to the destination |
| grROPWhite | A value of 1, or black is drawn to the destination |


### getDrawMode()
This method returns the current draw mode for the library. The draw mode determines how pixels are set on the screen during drawing operations. 

```c++
grRasterOp_t getDrawMode(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| return value | `grRasterOp_t` | The current aster operation (ROP) of the graphics system.|
