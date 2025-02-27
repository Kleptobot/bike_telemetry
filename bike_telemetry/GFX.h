// 'Left', 16x16px
const unsigned char epd_bitmap_left [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x18, 0x00, 0x3f, 0xf8, 0x7f, 0xf8, 
  0x7f, 0xf8, 0x3f, 0xf8, 0x18, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'right', 16x16px
const unsigned char epd_bitmap_right [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x18, 0x1f, 0xfc, 0x1f, 0xfe, 
  0x1f, 0xfe, 0x1f, 0xfc, 0x00, 0x18, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'UP', 16x16px
const unsigned char epd_bitmap_UP [] PROGMEM = {
  0x00, 0x00, 0x01, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 
  0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'Down', 16x16px
const unsigned char epd_bitmap_Down [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 
  0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x0f, 0xf0, 0x07, 0xe0, 0x03, 0xc0, 0x01, 0x80, 0x00, 0x00
};

// 'pause', 16x16px
const unsigned char epd_bitmap_pause [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x1e, 0x78, 0x1e, 0x78, 0x1e, 0x78, 0x1e, 0x78, 0x1e, 0x78, 0x1e, 0x78, 
  0x1e, 0x78, 0x1e, 0x78, 0x1e, 0x78, 0x1e, 0x78, 0x1e, 0x78, 0x1e, 0x78, 0x00, 0x00, 0x00, 0x00
};
// 'play', 16x16px
const unsigned char epd_bitmap_play [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x3c, 0x00, 0x3f, 0x00, 0x3f, 0xc0, 0x3f, 0xf0, 0x3f, 0xf8, 
  0x3f, 0xf8, 0x3f, 0xf0, 0x3f, 0xc0, 0x3f, 0x00, 0x3c, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'stop', 16x16px
const unsigned char epd_bitmap_stop [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 
	0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'Bluetooth', 16x16px
const unsigned char epd_bitmap_Bluetooth [] PROGMEM = {
	0x00, 0x00, 0x01, 0x00, 0x01, 0x80, 0x01, 0x40, 0x09, 0x20, 0x05, 0x40, 0x03, 0x80, 0x01, 0x00, 
	0x03, 0x80, 0x05, 0x40, 0x09, 0x20, 0x01, 0x40, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'spanner', 16x16px
const unsigned char epd_bitmap_spanner [] PROGMEM = {
  0x00, 0x00, 0x00, 0xe0, 0x01, 0xc0, 0x03, 0x80, 0x03, 0x84, 0x03, 0xcc, 0x07, 0xfc, 0x0f, 0xf8, 
  0x1f, 0xf0, 0x3f, 0x80, 0x7f, 0x00, 0x7e, 0x00, 0x3c, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'gear', 16x16px
const unsigned char epd_bitmap_gear [] PROGMEM = {
	0x03, 0xc0, 0x12, 0x58, 0x2c, 0x2c, 0x40, 0x04, 0x20, 0x04, 0x23, 0xc4, 0xc6, 0x63, 0x84, 0x21, 
	0x84, 0x21, 0xc6, 0x63, 0x23, 0xc4, 0x40, 0x04, 0x60, 0x04, 0x3c, 0x38, 0x02, 0x40, 0x03, 0xc0
};
// 'save', 16x16px
const unsigned char epd_bitmap_save [] PROGMEM = {
  0x00, 0x00, 0x1f, 0xe0, 0x3f, 0xf0, 0x24, 0x78, 0x27, 0xc8, 0x23, 0x8c, 0x20, 0x04, 0x27, 0xe4, 
  0x2f, 0xf4, 0x28, 0x14, 0x28, 0x14, 0x28, 0x14, 0x28, 0x14, 0x3f, 0xfc, 0x1f, 0xf8, 0x00, 0x00
};
// 'thermo', 16x16px
const unsigned char epd_bitmap_thermo [] PROGMEM = {
  0x01, 0x80, 0x01, 0x80, 0x03, 0xc0, 0x02, 0x40, 0x02, 0xc0, 0x02, 0xc0, 0x02, 0x40, 0x02, 0xc0, 
  0x06, 0x60, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x06, 0x60, 0x03, 0xc0, 0x00, 0x00
};
// 'clock', 16x16px
const unsigned char epd_bitmap_clock [] PROGMEM = {
	0x00, 0x00, 0x07, 0xe0, 0x08, 0x10, 0x11, 0x08, 0x21, 0x04, 0x41, 0x04, 0x41, 0x02, 0x41, 0x02, 
	0x41, 0xe2, 0x40, 0x02, 0x40, 0x04, 0x20, 0x04, 0x10, 0x08, 0x0c, 0x30, 0x03, 0xc0, 0x00, 0x00
};
// 'loop', 16x16px
const unsigned char epd_bitmap_loop [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x20, 0x04, 0x40, 0x02, 0x40, 0x02, 
	0x40, 0x02, 0x44, 0x02, 0x22, 0x04, 0x1d, 0x58, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'power', 16x16px
const unsigned char epd_bitmap_power [] PROGMEM = {
	0x00, 0x00, 0x01, 0x80, 0x0d, 0xb0, 0x1d, 0xb8, 0x31, 0x8c, 0x71, 0x8c, 0x60, 0x06, 0x60, 0x06, 
	0x60, 0x06, 0x60, 0x06, 0x70, 0x0e, 0x30, 0x0c, 0x1c, 0x38, 0x0f, 0xf0, 0x07, 0xe0, 0x00, 0x00
};
// 'down-right', 16x16px
const unsigned char epd_bitmap_down_right [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38, 0x00, 0x39, 0x80, 0x38, 0xc0, 0x38, 0x70, 
	0x3f, 0xf8, 0x3f, 0xfc, 0x3f, 0xf8, 0x00, 0x70, 0x00, 0xc0, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};
// 'heart', 16x16px
const unsigned char epd_bitmap_heart [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x70, 0x08, 0x88, 0x10, 0x04, 0x26, 0x02, 0x24, 0x02, 
	0x10, 0x04, 0x08, 0x08, 0x04, 0x10, 0x02, 0x20, 0x01, 0x40, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00
};
// 'antenna', 16x16px
const unsigned char epd_bitmap_antenna [] PROGMEM = {
	0x08, 0x00, 0x1c, 0x00, 0x26, 0x00, 0x43, 0x00, 0xc1, 0x00, 0x61, 0x80, 0x31, 0xc0, 0x1f, 0xe0, 
	0x03, 0xf8, 0x11, 0x8c, 0xd8, 0x86, 0xdc, 0x83, 0x4e, 0xc2, 0x60, 0x64, 0x3c, 0x38, 0x0c, 0x10
};
// 'battery', 32x16px
const unsigned char epd_bitmap_battery [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xf0, 0x18, 0x00, 0x00, 0x30, 
	0x18, 0x00, 0x00, 0x30, 0x18, 0x00, 0x00, 0x3c, 0x18, 0x00, 0x00, 0x3c, 0x18, 0x00, 0x00, 0x3c, 
	0x18, 0x00, 0x00, 0x3c, 0x18, 0x00, 0x00, 0x3c, 0x18, 0x00, 0x00, 0x3c, 0x18, 0x00, 0x00, 0x30, 
	0x18, 0x00, 0x00, 0x30, 0x1f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'left_arrow_large', 32x32px
const unsigned char epd_bitmap_left_arrow_large [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 
	0x00, 0x0f, 0xc0, 0x00, 0x00, 0x1f, 0xc0, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x7f, 0xc0, 0x00, 
	0x00, 0xff, 0x80, 0x00, 0x01, 0xff, 0x00, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x07, 0xfc, 0x00, 0x00, 
	0x0f, 0xf8, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xff, 0xfe, 0x1f, 0xf8, 0x00, 0x00, 
	0x0f, 0xfc, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x03, 0xff, 0x00, 0x00, 0x01, 0xff, 0x80, 0x00, 
	0x00, 0xff, 0xc0, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x1f, 0xc0, 0x00, 
	0x00, 0x07, 0x80, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'bluetooth_large', 32x32px
const unsigned char epd_bitmap_bluetooth_large [] PROGMEM = {
	0x00, 0x00, 0xc0, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x01, 0xf0, 0x00, 0x00, 0x01, 0xf8, 0x00, 
	0x00, 0x01, 0xfc, 0x00, 0x01, 0x81, 0xdc, 0x00, 0x03, 0xc1, 0xce, 0x00, 0x01, 0xe1, 0xc7, 0x00, 
	0x00, 0xe1, 0xc7, 0x80, 0x00, 0x71, 0xc7, 0x80, 0x00, 0x39, 0xcf, 0x00, 0x00, 0x3d, 0xde, 0x00, 
	0x00, 0x1f, 0xfc, 0x00, 0x00, 0x0f, 0xf8, 0x00, 0x00, 0x07, 0xf0, 0x00, 0x00, 0x03, 0xe0, 0x00, 
	0x00, 0x03, 0xe0, 0x00, 0x00, 0x07, 0xf0, 0x00, 0x00, 0x0f, 0xf8, 0x00, 0x00, 0x1f, 0xfc, 0x00, 
	0x00, 0x3d, 0xde, 0x00, 0x00, 0x39, 0xcf, 0x00, 0x00, 0x71, 0xc7, 0x80, 0x00, 0xe1, 0xc7, 0x80, 
	0x01, 0xe1, 0xc7, 0x00, 0x03, 0xc1, 0xce, 0x00, 0x01, 0x81, 0xdc, 0x00, 0x00, 0x01, 0xfc, 0x00, 
	0x00, 0x01, 0xf8, 0x00, 0x00, 0x01, 0xf0, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0xc0, 0x00
};
// 'clock_large', 32x32px
const unsigned char epd_bitmap_clock_large [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x7f, 0xfc, 0x00, 
	0x00, 0xfd, 0x3f, 0x00, 0x01, 0xe1, 0x07, 0x80, 0x03, 0x81, 0x03, 0xc0, 0x07, 0x00, 0x01, 0xe0, 
	0x0e, 0x00, 0x02, 0xe0, 0x0e, 0x00, 0x04, 0x70, 0x1c, 0x00, 0x08, 0x30, 0x1c, 0x08, 0x10, 0x38, 
	0x18, 0x0c, 0x20, 0x38, 0x18, 0x06, 0x60, 0x18, 0x38, 0x03, 0xc0, 0x18, 0x3f, 0x03, 0x80, 0xf8, 
	0x38, 0x01, 0x80, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x38, 0x1c, 0x00, 0x00, 0x38, 
	0x1c, 0x00, 0x00, 0x30, 0x0e, 0x00, 0x00, 0x70, 0x0e, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x01, 0xe0, 
	0x03, 0x81, 0x03, 0xc0, 0x01, 0xe1, 0x07, 0x80, 0x00, 0xfd, 0x3f, 0x00, 0x00, 0x7f, 0xfc, 0x00, 
	0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'heart_large', 32x32px
const unsigned char epd_bitmap_heart_large [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0xc0, 0x07, 0x00, 0x07, 0xf8, 0x3f, 0xc0, 0x0c, 0x0c, 0x60, 0x30, 0x18, 0x02, 0xc0, 0x18, 
	0x30, 0x03, 0x80, 0x08, 0x20, 0x01, 0x00, 0x0c, 0x60, 0x30, 0x00, 0x04, 0x60, 0x30, 0x00, 0x04, 
	0x60, 0x38, 0x00, 0x04, 0x60, 0x68, 0x18, 0x04, 0x20, 0x4c, 0x18, 0x0c, 0x3f, 0xc4, 0x1f, 0xfc, 
	0x30, 0x04, 0x36, 0x08, 0x10, 0x06, 0x24, 0x18, 0x18, 0x02, 0x60, 0x10, 0x0c, 0x03, 0x40, 0x20, 
	0x04, 0x01, 0xc0, 0x60, 0x02, 0x01, 0xc0, 0xc0, 0x01, 0x80, 0x81, 0x80, 0x00, 0xc0, 0x03, 0x00, 
	0x00, 0x60, 0x0c, 0x00, 0x00, 0x18, 0x38, 0x00, 0x00, 0x0e, 0xe0, 0x00, 0x00, 0x03, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'antenna_large', 32x32px
const unsigned char epd_bitmap_antenna_large [] PROGMEM = {
	0x00, 0x80, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x03, 0xe0, 0x00, 0x00, 0x07, 0x70, 0x00, 0x00, 
	0x0e, 0x38, 0x00, 0x00, 0x1c, 0x1c, 0x00, 0x00, 0x38, 0x0e, 0x00, 0x00, 0x70, 0x07, 0x00, 0x00, 
	0xe0, 0x03, 0x00, 0x00, 0x70, 0x03, 0x00, 0x00, 0x38, 0x03, 0x00, 0x00, 0x1c, 0x03, 0xe0, 0x00, 
	0x0e, 0x03, 0xf0, 0x00, 0x07, 0x07, 0xf8, 0x00, 0x03, 0xff, 0xf8, 0x00, 0x01, 0xff, 0xf8, 0x00, 
	0x00, 0x0f, 0xff, 0x80, 0x00, 0x0f, 0xff, 0xc0, 0x03, 0x07, 0xe0, 0xe0, 0x03, 0x83, 0xc0, 0x70, 
	0x63, 0x80, 0xc0, 0x38, 0x63, 0x80, 0xc0, 0x1c, 0x73, 0xc0, 0xc0, 0x0e, 0x71, 0xf8, 0xc0, 0x07, 
	0x70, 0xfc, 0xe0, 0x0e, 0x38, 0x7c, 0x70, 0x1c, 0x3c, 0x00, 0x38, 0x38, 0x1e, 0x00, 0x1c, 0x70, 
	0x0f, 0xc0, 0x0e, 0xe0, 0x07, 0xf0, 0x07, 0xc0, 0x01, 0xf0, 0x03, 0x80, 0x00, 0x00, 0x01, 0x00
};