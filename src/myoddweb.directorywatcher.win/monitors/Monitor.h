#pragma once

class Monitor
{
public:
  Monitor( __int64 id );
  virtual ~Monitor();

  __int64 Id() const;

private:
  const __int64 _id;
};