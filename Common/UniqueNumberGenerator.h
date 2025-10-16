#pragma once
#include <QSet>
#include <QRandomGenerator>
class UniqueNumberGenerator {
public:
    int generate() {
        while (true) {
            int num = QRandomGenerator::global()->bounded(100000, 1000000);
            // 保证是6位数字 (100000 ~ 999999)
            if (!used.contains(num)) {
                used.insert(num);
                return num;
            }
            // 否则继续随机
        }
    }

private:
    QSet<int> used; // 存已生成的数字
};