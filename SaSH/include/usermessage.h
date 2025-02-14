#pragma once
enum UserMessage
{
	kUserMessage = 0x400 + 0x1024,
	kConnectionOK,
	kInitialize,
	kUninitialize,
	kGetModule,
	kSendPacket,

	//
	kEnableEffect,
	kEnableCharShow,
	kSetTimeLock,
	kEnableSound,
	kEnableImageLock,
	kEnablePassWall,
	kEnableFastWalk,
	kSetBoostSpeed,
	kEnableMoveLock,
	kMuteSound,
	kEnableBattleDialog,
	kSetGameStatus,
	kSetWorldStatus,
	kSetBlockPacket,
	kBattleTimeExtend,
	kEnableOptimize,
	kEnableWindowHide,
	kECBCrypt,
	kECBEncrypt,
	kEnableForwardSend,

	//Action
	kSendAnnounce,
	kSetMove,
	kDistoryDialog,
	kCleanChatHistory,
	kCreateDialog,
	kResetCharObject,
};