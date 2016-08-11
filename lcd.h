extern void InitLCD(void) ;
extern void DrawPixel(unsigned char x, unsigned char y) ; // 128 x 64
extern void ClearPixel(unsigned char x, unsigned char y) ; // 128 x 64
extern void ClearLCD(void) ;
extern void UpdateScreen(void) ;
extern void PutcharLCD(char) ;
extern void Backlight(unsigned long) ;
extern void CursorPos(unsigned char x, unsigned char y) ; // 21 x 8 chars

// Note the screen is 128 x 64 pixels
// Note the screen allows has 21 x 8 characters
