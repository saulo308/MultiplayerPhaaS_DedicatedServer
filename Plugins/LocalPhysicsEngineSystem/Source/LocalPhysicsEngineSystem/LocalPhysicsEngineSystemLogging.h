#pragma once

#include "Stats/Stats.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLPES, Log, All);

#define LPES_LOG_INFO(FMT, ...) UE_LOG(LogLPES, Log, (FMT), ##__VA_ARGS__)
#define LPES_LOG_WARNING(FMT, ...) UE_LOG(LogLPES, Warning, (FMT), ##__VA_ARGS__)
#define LPES_LOG_ERROR(FMT, ...) UE_LOG(LogLPES, Error, (FMT), ##__VA_ARGS__)