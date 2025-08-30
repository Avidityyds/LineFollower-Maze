# Line Follower & Maze Robot (ATmega328P, AVR)

這個專案收錄了微控制器期末專題「循跡＆走迷宮自走車」的程式：

- `src/LineFollow.cpp` – 以左右 IR 感測器差值為誤差的 **P 控制循跡**  
- `src/Maze.cpp` – **DMS（距離感測器）+ IR** 的撞牆偵測策略，沿牆前進、卡牆時自動倒退  

▶️ Demo 影片： [YouTube Shorts](https://youtube.com/shorts/3_DuWfTLTRo?si=_PDdgESB-6ADxS3Z)

---

## 硬體/環境
- 微控制器：ATmega328P    
- 開發工具：Microchip Studio (Atmel Studio)  
- 感測器：
  - IR 感測器 × 2 (循跡)
  - DMS 距離感測器 × 1 + IR 感測器 × 1 (迷宮)

---


