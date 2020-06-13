#pragma once
#include "pch.h"

using myoddweb::directorywatcher::Request;
using myoddweb::directorywatcher::EventCallback;
using myoddweb::directorywatcher::StatisticsCallback;
using myoddweb::directorywatcher::LoggerCallback;

class RequestHelper : public ::Request
{
public:
  RequestHelper(
    const wchar_t* path, 
    bool recursive, 
    const LoggerCallback& loggerCallback, 
    const EventCallback& eventsCallback, 
    const StatisticsCallback& statisticsCallback, 
    long long eventsCallbackRateMs, 
    long long statisticsCallbackRateMs
  ) : Request(
    path,
    recursive,
    loggerCallback,
    eventsCallback,
    statisticsCallback,
    eventsCallbackRateMs,
    statisticsCallbackRateMs
  )
  {
    
  }
};