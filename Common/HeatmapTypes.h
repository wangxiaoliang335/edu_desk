#pragma once
#include <QColor>

// 分段区间结构
struct SegmentRange {
    double minValue;  // 最小值
    double maxValue;  // 最大值
    QColor color;     // 颜色
    int count;        // 人数
    double percentage; // 百分数
};

