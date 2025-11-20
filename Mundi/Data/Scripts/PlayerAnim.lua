-- 상태 상수 정의 (가독성을 위해)
local STATE_IDLE = 0
local STATE_WALK = 1
local STATE_RUN  = 2
local STATE_JUMP = 3

-- 현재 상태를 저장할 변수 (초기값: IDLE)
local CurrentState = STATE_IDLE

-- 속도 기준값 (엔진 단위에 맞춰 조절하세요)
local SPEED_STOP_THRESHOLD = 0.5   -- 이보다 느리면 멈춤
local SPEED_RUN_THRESHOLD  = 4 -- 이보다 빠르면 달리기

local IsInAir = false

function Initialize()
    -- 시작할 때 기본 포즈 잡아주기 (강제 리셋)
    PlayAnimationByName("Data/Model/Animation/Idle.fbx", true, 1.0, true)
    CurrentState = STATE_IDLE
end

function Jump()
    IsInAir = true
    coroutine.yield("wait_time", 1.0)
    IsInAir = false
end

function UpdateAnimation(DeltaTime)
    -- 1. 현재 속도 가져오기
    local Speed = GetFloat("Speed")
    -- IsInAir = GetBool("IsInAir")

    -- 하드코딩으로 점프 애니메이션 구현
    if InputManager:IsKeyDown(' ') then
        StartCoroutine(Jump)
    end

    -- 2. 속도에 따른 '목표 상태(TargetState)' 결정
    local TargetState = CurrentState -- 기본적으로 유지

    -- [최우선 순위] 공중에 있으면 무조건 점프 상태
    if IsInAir then
        TargetState = STATE_JUMP
    else
        if Speed < SPEED_STOP_THRESHOLD then
            TargetState = STATE_IDLE
        elseif Speed < SPEED_RUN_THRESHOLD then
            TargetState = STATE_WALK
        else
            TargetState = STATE_RUN
        end
    end

    -- 3. 상태가 바뀌었을 때만 BlendToAnimation 실행 (State Machine Transition)
    if TargetState ~= CurrentState then

        if TargetState == STATE_JUMP then
            -- 땅 -> 공중: 빠르게 전환 (0.1초), 반복 재생 끔(false)
            BlendToAnimation("Data/Model/Animation/Jump.fbx", 0.9, false, 1.0)

        elseif TargetState == STATE_IDLE then
            -- 걷기/뛰기 -> 멈춤 (0.3초 동안 부드럽게)
            BlendToAnimation("Data/Model/Animation/Idle.fbx", 0.3, true, 1.0)

        elseif TargetState == STATE_WALK then
            -- 멈춤/뛰기 -> 걷기 (0.2초 동안 부드럽게)
            BlendToAnimation("Data/Model/Animation/Walk.fbx", 0.2, true, 1.0)

        elseif TargetState == STATE_RUN then
            -- 걷기 -> 뛰기 (0.2초 동안 빠르게 전환)
            BlendToAnimation("Data/Model/Animation/Run.fbx", 0.2, true, 1.0)
        end

        -- 상태 업데이트 (중복 호출 방지)
        CurrentState = TargetState
        
        -- 디버깅용 (필요시 주석 해제)
        -- print("State Changed to: " .. TargetState .. " (Speed: " .. Speed .. ")")
    end
end
