#pragma once
#include "cocos2d.h"
class LoadingScene : public cocos2d::Scene
{
public:
	virtual bool init();
	void onEnter();  //override函数
	CREATE_FUNC(LoadingScene);
private:
	void loadingCallBack(cocos2d::Texture2D *texture); //加载回调函数
};