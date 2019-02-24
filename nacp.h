#ifndef NXCI_NACP_H
#define NXCI_NACP_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    char Name[0x200];
    char Publisher[0x100];
} nacp_application_title_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    nacp_application_title_t Title[0x10];
    char Isbn[0x25];
    uint8_t StartupUserAccount;
    uint8_t UserAccountSwitchLock;
    uint8_t AddOnContentRegistrationType;
    uint32_t AttributeFlag;
    uint32_t SupportedLanguageFlag;
    uint32_t ParentalControlFlag;
    uint8_t Screenshot;
    uint8_t VideoCapture;
    uint8_t DataLossConfirmation;
    uint8_t PlayLogPolicy;
    uint64_t PresenceGroupId;
    uint8_t RatingAge[0x20];
    char DisplayVersion[0x10];
    uint64_t AddOnContentBaseId;
    uint64_t SaveDataOwnerId;
    uint64_t UserAccountSaveDataSize;
    uint64_t UserAccountSaveDataJournalSize;
    uint64_t DeviceSaveDataSize;
    uint64_t DeviceSaveDataJournalSize;
    uint64_t BcatDeliveryCacheStorageSize;
    char ApplicationErrorCodeCategory[0x8];
    uint64_t LocalCommunicationId[0x8];
    uint8_t LogoType;
    uint8_t LogoHandling;
    uint8_t RuntimeAddOnContentInstall;
    uint8_t _0x30F3[0x3];
    uint8_t CrashReport;
    uint8_t Hdcp;
    uint64_t SeedForPseudoDeviceId ;
    char BcatPassphrase[0x41];
    uint8_t _0x3141;
    uint8_t ReservedForUserAccountSaveDataOperation[0x6];
    uint64_t UserAccountSaveDataSizeMax;
    uint64_t UserAccountSaveDataJournalSizeMax;
    uint64_t DeviceSaveDataSizeMax;
    uint64_t DeviceSaveDataJournalSizeMax;
    uint64_t TemporaryStorageSize;
    uint64_t CacheStorageSize;
    uint64_t CacheStorageJournalSize;
    uint64_t CacheStorageDataAndJournalSizeMax;
    uint64_t CacheStorageIndexMax;
    uint64_t PlayLogQueryableApplicationId[0x10];
    uint8_t PlayLogQueryCapability;
    uint8_t RepairFlag;
    uint8_t ProgramIndex;
    uint8_t RequiredNetworkServiceLicenseOnLaunchFlag;
    uint8_t Reserved[0xDEC];
} nacp_t;
#pragma pack(pop)

#endif