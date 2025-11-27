#pragma once

// ÉVÅ[ÉìéÌï 
enum SCENE 
{
	SCENE_NONE = 0,    
    SCENE_LOGO,
    SCENE_TITLE,
    SCENE_GAME,
    SCENE_RESULT
};

void SceneManager_Initialize();
void SceneManager_Finalize();
void SceneManager_Update();
void SceneManager_Draw();

void SetScene(SCENE scene);