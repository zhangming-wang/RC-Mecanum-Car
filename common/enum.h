#pragma once

namespace MotionService {
    enum Type {
        Idle = 0,
        HeartBeat,
        SwitchToBluetooth,
        Restart,
        Brake,
        StopMove,
        MoveFront,
        MoveBack,
        MoveLeft,
        MoveRight,
        MoveLeftFront,
        MoveLeftBack,
        MoveRightFront,
        MoveRightBack,
        TurnLeft,
        TurnRight,

        SetSpeedPercent,
        SetSpeedPlanState,
        SetEnablePubMotionStatus,

        ReadParams,
        WriteParams,
        SaveParams,

        ReadConfig,
        WriteConfig,
        SaveConfig,

        SwitchToPS3,
    };
}