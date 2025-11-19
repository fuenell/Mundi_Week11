function Initialize()

end

function UpdateAnimation(DeltaTime)
    local Speed = GetFloat("Speed")

    -- 이동 상태
    if Speed < 0.01 then
        PlayAnimationByName("Data/Model/Animation/Idle.fbx", true, 1.0, false)
    elseif Speed < 0.1 then
        PlayAnimationByName("Data/Model/Animation/Walk.fbx", true, 1.0, false)
    else
        PlayAnimationByName("Data/Model/Animation/Run.fbx", true, 1.0, false)
    end
end