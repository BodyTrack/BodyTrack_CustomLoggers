/*
 *  uploader.h
 *  BaseStation
 *
 *  Created by Joshua Schapiro on 7/8/11.
 *  Copyright 2011 Carnegie Mellon. All rights reserved.
 *
 */

bool Uploader_Update(void);
void Uploader_connectToComputer(void);
bool Uploader_getTime(void);
void Uploader_sendSSID(void);
void Uploader_sendAuthType(void);
void Uploader_sendKey(void);
void Uploader_sendUser(void);
void Uploader_sendNickname(void);
void Uploader_sendFilename(void);
bool Uploader_uploadFile(void);
bool Uploader_eraseFile(void);
void Uploader_sendServer(void);
void Uploader_sendPort(void);

void Uploader_ClearCRC(void);
void Uploader_WriteCRC(void);
void Uploader_UpdateCRC(uint8_t byte);