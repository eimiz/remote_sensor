#pragma once
void uartsimInit();
void uartsimEnableNVICint();
void uartsimEnableInt();
void uartsimDisableInt();
void uartsimSend(uint8_t data);
uint8_t uartsimRead();
void uartsimSendBuf(const char *lbuf, int len);
