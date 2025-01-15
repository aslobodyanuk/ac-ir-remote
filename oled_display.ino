void updateScreen() {
  oled.home();  

  oled.setScale(3);
  oled.setCursor(0, 0);
  oled.print(temperature, 1);
  oled.print("c");

  oled.setScale(1);

  printStatus(0, 5);

  oled.setCursor(0, 6);
  if (isThresholdChange) {
    oled.print(" ");
  } else {
    oled.print("*");
  }
  oled.print("Set: ");
  oled.print(data.setTemperature);
  oled.print("c");
  
  oled.setCursor(0, 7);
  if (isThresholdChange) {
    oled.print("*");
  } else {
    oled.print(" ");
  }
  oled.print("Thr: ");
  oled.print(data.temperatureThreshold);

  oled.setCursor(80, 7);
  if (isAcOn) {
    oled.print("AC ON ");
  } else {
    oled.print("AC OFF");
  }

  oled.rect(124, 60, 126, 62, screenHearbeatState);
  screenHearbeatState = !screenHearbeatState;

  drawEnabledIcon();

  oled.update();
}

void printStatus(int x, int y) {
  oled.setCursor(x, y);

  if (isFirstStartup) {
    oled.print("STARTUP           ");
    return;
  }

  if (isTempError) {
    oled.print("TEMP SENSOR ERROR ");
    oled.print(tempErrorCode);
    return;
  }

  oled.print("OK                 ");
}

const unsigned char powerOnIcon [] PROGMEM = {
	0xff, 0xff, 0x7f, 0x3f, 0x1f, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x01, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xbf, 0x1f, 0x3f, 0x7f, 0xff, 0xff, 0xff, 0xe7, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xe0, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xe7, 0xff, 
	0xff, 0xff, 0xfe, 0xfc, 0xf0, 0xf1, 0xe3, 0xc7, 0xcf, 0xcf, 0xcf, 0x8f, 0x8f, 0xcf, 0xcf, 0xcf, 
	0xc7, 0xe3, 0xf1, 0xf0, 0xfc, 0xfe, 0xff, 0xff
};

void drawEnabledIcon() {
  if (isEnabled) {
    oled.drawBitmap(100, 0, powerOnIcon, 24, 24, BITMAP_INVERT, BUF_ADD);
  } else {
    oled.rect(100, 0, 100 + 24, 0 + 24, 0);
  }  
}
