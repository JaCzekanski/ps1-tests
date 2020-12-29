#pragma once
#include <stdint.h>

void cdInit();

void cdClearParameterFifo();
void cdPushParameter(uint8_t param);
void cdCommand(uint8_t cmd);
uint8_t cdReadResponse();
uint8_t cdGetInt(bool ack = true);
uint8_t cdReadData();

void cdWantData(bool wantData);

bool cdCommandBusy();
bool cdResponseEmpty();
bool cdDataEmpty();