#pragma once
#include <QSet>
#include <QRandomGenerator>
class UniqueNumberGenerator {
public:
    int generate() {
        while (true) {
            int num = QRandomGenerator::global()->bounded(100000, 1000000);
            // ��֤��6λ���� (100000 ~ 999999)
            if (!used.contains(num)) {
                used.insert(num);
                return num;
            }
            // ����������
        }
    }

private:
    QSet<int> used; // �������ɵ�����
};