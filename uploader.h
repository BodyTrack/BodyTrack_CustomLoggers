/*
 *  uploader.h
 *  BaseStation
 *
 *  Created by Joshua Schapiro on 7/8/11.
 *  Copyright 2011 Carnegie Mellon. All rights reserved.
 *
 */

bool Uploader_Update(void);
bool Uploader_connectToComputer(void);
bool Uploader_getTime(void);
bool Uploader_sendSSID(void);
bool Uploader_sendAuthType(void);
bool Uploader_sendKey(void);
bool Uploader_sendUser(void);
bool Uploader_sendNickname(void);
bool Uploader_sendFilename(void);
bool Uploader_uploadFile(void);
bool Uploader_eraseFile(void);
bool Uploader_sendServer(void);
bool Uploader_sendPort(void);

void Uploader_ClearCRC(void);
bool Uploader_WriteCRC(void);
void Uploader_UpdateCRC(uint8_t byte);