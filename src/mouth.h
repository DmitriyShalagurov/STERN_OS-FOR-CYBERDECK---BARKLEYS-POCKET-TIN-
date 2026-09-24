#ifndef MOUTH_H
#define MOUTH_H

// Одиночный запуск "рта" (для команды mouth)
void runMouth();

// Режим фоновой анимации (для меню и других циклов)
void mouthBegin();
void mouthUpdate();
void mouthTrigger(float amount);   // 0.0 .. 1.5
void mouthEnd();

#endif