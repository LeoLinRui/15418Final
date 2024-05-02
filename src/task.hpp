#include "main.hpp"

TaskGroup phaseZeroTasks;
phaseZeroTasks.sendTasks = {
    Task(Operation::Send, Neighbor::Right, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_maxX() - 2 * r);
        bbox.set_minY(b.get_minY());
        bbox.set_maxX(b.get_maxX());
        bbox.set_maxY(b.get_maxY() - 2 * r);
        return bbox;
    }),
    Task(Operation::Send, Neighbor::BR, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_maxX() - 2 * r);
        bbox.set_minY(b.get_maxY() - 2 * r);
        bbox.set_maxX(b.get_maxX());
        bbox.set_maxY(b.get_maxY());
        return bbox;
    }),
    Task(Operation::Send, Neighbor::Bottom, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX());
        bbox.set_minY(b.get_maxY() - 2 * r);
        bbox.set_maxX(b.get_maxX() - 2 * r);
        bbox.set_maxY(b.get_maxY());
        return bbox;
    })
};
phaseZeroTasks.receiveTasks = {
    Task(Operation::Receive, Neighbor::Left, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX() - 2 * r);
        bbox.set_minY(b.get_minY());
        bbox.set_maxX(b.get_minX());
        bbox.set_maxY(b.get_maxY() - 2 * r);
        return bbox;
    }),
    Task(Operation::Receive, Neighbor::TL, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_maxX() - 2 * r);
        bbox.set_minY(b.get_minY() - 2 * r);
        bbox.set_maxX(b.get_minX());
        bbox.set_maxY(b.get_minY());
        return bbox;
    }),
    Task(Operation::Receive, Neighbor::Top, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX());
        bbox.set_minY(b.get_minY() - 2 * r);
        bbox.set_maxX(b.get_maxX() - 2 * r);
        bbox.set_maxY(b.get_minY());
        return bbox;
    })
};
phaseZeroTasks.refineTask = std::nullopt;

TaskGroup phaseOneTasks;
phaseOneTasks.sendTasks = {
    Task(Operation::Send, Neighbor::Left, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX() - 2 * r);
        bbox.set_minY(b.get_minY() - 2 * r);
        bbox.set_maxX(b.get_minX() + 2 * r);
        bbox.set_maxY((b.get_minY() + b.get_maxY()) / 2 + 2 * r);
        return bbox;
    })
};
phaseOneTasks.receiveTasks = {
    Task(Operation::Receive, Neighbor::Right, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_maxX() - 2 * r);
        bbox.set_minY(b.get_minY() - 2 * r);
        bbox.set_maxX(b.get_maxX() + 2 * r);
        bbox.set_maxY((b.get_minY() + b.get_maxY()) / 2 + 2 * r);
        return bbox;
    })
};
phaseOneTasks.refineTask = 
    Task(Operation::Refine, [](Bbox2* b, double r) { 
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX() - r);
        bbox.set_minY(b.get_minY() - r);
        bbox.set_maxX((b.get_minX() + b.get_maxX()) / 2 + r);
        bbox.set_maxY((b.get_minY() + b.get_maxY()) / 2 + r);
        return bbox;
    });

TaskGroup phaseTwoTasks;
phaseTwoTasks.sendTasks = {
    Task(Operation::Send, Neighbor::Top, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX() + 2 * r);
        bbox.set_minY(b.get_minY() - 2 * r);
        bbox.set_maxX(b.get_maxX() + 2 * r);
        bbox.set_maxY(b.get_minY() + 2 * r);
        return bbox;
    })
};
phaseTwoTasks.receiveTasks = {
    Task(Operation::Receive, Neighbor::Bottom, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX() + 2 * r);
        bbox.set_minY(b.get_maxY() - 2 * r);
        bbox.set_maxX(b.get_maxX() + 2 * r);
        bbox.set_maxY(b.get_maxY() + 2 * r);
        return bbox;
    })
};
phaseTwoTasks.refineTask =
    Task(Operation::Refine, [](Bbox2* b, double r) { 
        Bbox2* bbox = Bbox2();
        bbox.set_minX((b.get_minX() + b.get_maxX()) / 2);
        bbox.set_minY(b.get_minY() - r);
        bbox.set_maxX(b.get_maxX());
        bbox.set_maxY((b.get_minY() + b.get_maxY()) / 2 + r);
        return bbox;
    });

TaskGroup phaseThreeTasks;
phaseThreeTasks.sendTasks = {
    Task(Operation::Send, Neighbor::Right, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_maxX() - 2 * r);
        bbox.set_minY(b.get_minY() + 2 * r);
        bbox.set_maxX(b.get_maxX() + 2 * r);
        bbox.set_maxY(b.get_maxY() + 2 * r);
        return bbox;
    })
};
phaseThreeTasks.receiveTasks = {
    Task(Operation::Receive, Neighbor::Left, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX() - 2 * r);
        bbox.set_minY(b.get_minY() + 2 * r);
        bbox.set_maxX(b.get_minX() + 2 * r);
        bbox.set_maxY(b.get_maxY() + 2 * r);
        return bbox;
    })
};
phaseThreeTasks.refineTask =
    Task(Operation::Refine, [](Bbox2* b, double r) { 
        Bbox2* bbox = Bbox2();
        bbox.set_minX((b.get_minX() + b.get_maxX()) / 2 - r);
        bbox.set_minY((b.get_minY() + b.get_maxY()) / 2);
        bbox.set_maxX(b.get_maxX() + r);
        bbox.set_maxY(b.get_maxY());
        return bbox;
    });

TaskGroup phaseFourTasks;
phaseFourTasks.sendTasks = {
    Task(Operation::Send, Neighbor::Left, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX() - 2 * r);
        bbox.set_minY(b.get_minY() + 2 * r);
        bbox.set_maxX(b.get_minX());
        bbox.set_maxY(b.get_maxY());
        return bbox;
    }),
    Task(Operation::Send, Neighbor::BL, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX() - 2 * r);
        bbox.set_minY(b.get_maxY());
        bbox.set_maxX(b.get_minX());
        bbox.set_maxY(b.get_maxY() + 2 * r);
        return bbox;
    }),
    Task(Operation::Send, Neighbor::Bottom, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX());
        bbox.set_minY(b.get_maxY());
        bbox.set_maxX(b.get_maxX() - 2 * r);
        bbox.set_maxY(b.get_maxY() + 2 * r);
        return bbox;
    })
};
phaseFourTasks.receiveTasks = {
    Task(Operation::Receive, Neighbor::Right, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_maxX() - 2 * r);
        bbox.set_minY(b.get_minY() + 2 * r);
        bbox.set_maxX(b.get_maxX());
        bbox.set_maxY(b.get_maxY());
        return bbox;
    }),
    Task(Operation::Receive, Neighbor::TR, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_maxX() - 2 * r);
        bbox.set_minY(b.get_minY());
        bbox.set_maxX(b.get_maxX());
        bbox.set_maxY(b.get_minY() + 2 * r);
        return bbox;
    }),
    Task(Operation::Receive, Neighbor::Top, [](Bbox2* b, double r) {
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX());
        bbox.set_minY(b.get_minY());
        bbox.set_maxX(b.get_maxX() - 2 * r);
        bbox.set_maxY(b.get_minY() + 2 * r);
        return bbox;
    })
};
phaseFourTasks.refineTask =
    Task(Operation::Refine, [](Bbox2* b, double r) { 
        Bbox2* bbox = Bbox2();
        bbox.set_minX(b.get_minX());
        bbox.set_minY((b.get_minY() + b.get_maxY()) / 2);
        bbox.set_maxX((b.get_minX() + b.get_maxX()) / 2);
        bbox.set_maxY(b.get_maxY());
        return bbox;
    });

std::vector<TaskGroup> taskGroups = {phaseZeroTasks, phaseOneTasks, phaseTwoTasks, phaseThreeTasks, phaseFourTasks};