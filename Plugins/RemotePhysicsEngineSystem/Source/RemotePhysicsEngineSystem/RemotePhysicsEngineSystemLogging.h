#pragma once

#include "Stats/Stats.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRPES, Log, All);

#define RPES_LOG_INFO(FMT, ...) UE_LOG(LogRPES, Log, (FMT), ##__VA_ARGS__)
#define RPES_LOG_WARNING(FMT, ...) UE_LOG(LogRPES, Warning, (FMT), ##__VA_ARGS__)
#define RPES_LOG_ERROR(FMT, ...) UE_LOG(LogRPES, Error, (FMT), ##__VA_ARGS__)