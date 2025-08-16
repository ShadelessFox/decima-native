#pragma once

class EntityUpdaterListener {
public:
    virtual ~EntityUpdaterListener(); // 0
    virtual void OnBeginUpdate() = 0; // 1
    virtual void OnEndUpdate() = 0; // 2
};
