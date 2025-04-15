#pragma once

typedef struct AFMsg
{
	QString fileName;  // 文件名
	QString afId;      // 唯一ID（文件名+创建时间）
	int insertrow = 0; // 列号
	int itemWidth = 0; // 保留拖拽后的长度
}_AFMsg;
