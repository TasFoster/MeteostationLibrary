#pragma once

#include "Task.h"

template <typename Obj,typename Funk, typename Arg>
class TaskWrapper : public Task {

    Obj& obj;
    Funk funk;
    Arg arg;
public:
    explicit TaskWrapper(Obj& obj, Funk funk, Arg arg)
        : obj(obj), funk(funk), arg(arg) {}

    void execute() override {
        (obj.*funk)(arg);
    }
};