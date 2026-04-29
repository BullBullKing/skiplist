/* ************************************************************************
> File Name:     main.cpp
> Author:        BullBullKing
> Created Time:  2026-04-27 13:11:30
> Description:   The main function for Skip List, demonstrating the usage of SkipList class
 ************************************************************************/
#include <iostream>
#include "skiplist.h"
#define FILE_PATH "./store/dumpFile"

int main() {

    // 键值中的key用int型，如果用其他类型，需要自定义比较函数
    // 而且如果修改key的类型，同时需要修改skipList.load_file函数
    SkipList<int, std::string> skipList(6);   // 创建最大层数为6的跳表
	skipList.put(1, "learn"); 
	skipList.put(3, "algorithm");
	skipList.put(7, "confirm"); 
	skipList.put(8, "follow"); 
	skipList.put(9, "subscribe"); 
	skipList.put(19, "algorithm not lost"); 
	skipList.put(19, "follow me"); 

    std::cout << "skipList size:" << skipList.size() << std::endl;

    skipList.dumpFile();

    // skipList.loadFile();

    skipList.find(9);
    skipList.find(18);


    skipList.displayList();

    skipList.remove(3);
    skipList.remove(7);

    std::cout << "skipList size:" << skipList.size() << std::endl;

    skipList.displayList();
}
