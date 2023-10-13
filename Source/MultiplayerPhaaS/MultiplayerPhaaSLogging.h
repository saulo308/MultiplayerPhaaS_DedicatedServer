#pragma once

#include "Stats/Stats.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMultiplayerPhaaS, Log, All);

#define MPHAAS_LOG_INFO(FMT, ...) UE_LOG(LogMultiplayerPhaaS, Log, (FMT), ##__VA_ARGS__)
#define MPHAAS_LOG_WARNING(FMT, ...) UE_LOG(LogMultiplayerPhaaS, Warning, (FMT), ##__VA_ARGS__)
#define MPHAAS_LOG_ERROR(FMT, ...) UE_LOG(LogMultiplayerPhaaS, Error, (FMT), ##__VA_ARGS__)

DECLARE_STATS_GROUP(TEXT("MultiplayerPhaaS"), STATGROUP_MultiplayerPhaaS, STATCAT_Advanced)