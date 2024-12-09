#pragma once

class Stopwatch {
   protected:
      unsigned long startTime;

   public:
      Stopwatch() : startTime(0) {}

      void start() {
         startTime = clock();
      }

      unsigned long end() {
         unsigned long tmp = startTime;
         startTime = 0;
         return clock() - tmp;
      }
};