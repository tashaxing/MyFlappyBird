/*
*game:flappy bird
*author:tashaxing
*time:2014/10/26
*/
#include "GameScene.h"
#include "SimpleAudioEngine.h"
USING_NS_CC;
using namespace CocosDenshion;
Scene* GameScene::createScene()
{
	
	//创建游戏场景
	auto gameScene = Scene::createWithPhysics(); //用物理世界初始化场景
	gameScene->getPhysicsWorld()->setGravity(Vec2(0, -900)); //设置重力场,重力加速度可以根据手感改小点

	//添加主游戏层
	auto gameLayer = GameScene::create();
	gameLayer->setPhysicWorld(gameScene->getPhysicsWorld()); //绑定物理世界
	gameScene->addChild(gameLayer);
	return gameScene;
}

bool GameScene::init()
{
	if (!Layer::init())
		return false;

	Size visibleSize = Director::getInstance()->getVisibleSize();
	Point visibleOrigin = Director::getInstance()->getVisibleOrigin();

	//初始化游戏状态
	gameStatus = GAME_READY;
	score = 0;

	

	//添加游戏背景
	Sprite *backGround = Sprite::createWithSpriteFrameName("bg.png");
	backGround->setPosition(visibleOrigin.x + visibleSize.width / 2, visibleOrigin.y + visibleSize.height / 2);
	this->addChild(backGround);
	//logo
	auto gameLogo = Sprite::createWithSpriteFrameName("bird_logo.png");
	gameLogo->setPosition(visibleOrigin.x + visibleSize.width / 2, visibleOrigin.y + visibleSize.height / 2+100);
	gameLogo->setName("logo");
	this->addChild(gameLogo);

	//添加管子
	createPipes();

	//小鸟
	birdSprite = Sprite::create();
	birdSprite->setPosition(visibleOrigin.x + visibleSize.width / 3, visibleOrigin.y + visibleSize.height / 2);
	this->addChild(birdSprite);
	auto birdAnim = Animate::create(AnimationCache::getInstance()->animationByName("birdAnimation"));
	birdSprite->runAction(RepeatForever::create(birdAnim));  //挥翅动画
	auto up = MoveBy::create(0.4f, Point(0, 8));
	auto upBack = up->reverse();
	if (gameStatus == GAME_READY)
	{
		swingAction = RepeatForever::create(Sequence::create(up, upBack, NULL));
		birdSprite->runAction(swingAction); //上下晃动动画
	}
    //小鸟绑定刚体
	auto birdBody = PhysicsBody::createCircle(BIRD_RADIUS); //将小鸟当成一个圆，懒得弄精确的轮廓线了
	birdBody->setDynamic(true);   //设置为可以被物理场所作用而动作
	birdBody->setContactTestBitmask(1); //必须设置这项为1才能检测到不同的物体碰撞
	birdBody->setGravityEnable(false);   //设置是否被重力影响,准备画面中不受重力影响
	birdSprite->setPhysicsBody(birdBody); //为小鸟设置刚体

	//添加两个land
	land1 = Sprite::createWithSpriteFrameName("land.png");
	land1->setAnchorPoint(Point::ZERO); 
	land1->setPosition(Point::ZERO); 
	this->addChild(land1, 10);  //置于最顶层
	land2 = Sprite::createWithSpriteFrameName("land.png");
	land2->setAnchorPoint(Point::ZERO);
	land2->setPosition(Point::ZERO);
	this->addChild(land2, 10);
	

	//设置地板刚体
	Node *groundNode = Node::create();
	auto groundBody = PhysicsBody::createBox(Size(visibleSize.width, land1->getContentSize().height));
	groundBody->setDynamic(false);
	groundBody->setContactTestBitmask(1);
	groundNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE); //物理引擎中的刚体只允许结点锚点设置为中心
	groundNode->setPhysicsBody(groundBody);
	groundNode->setPosition(visibleOrigin.x+visibleSize.width/2,land1->getContentSize().height/2);
	this->addChild(groundNode);

	//记分牌,在最外层，这里懒得弄图片文字了^_^
	scoreLabel = LabelTTF::create("0", "Felt", 35);
	scoreLabel->setPosition(visibleOrigin.x + visibleSize.width / 2, visibleOrigin.y + visibleSize.height / 2 + 200);
	scoreLabel->setVisible(false); //一开始隐藏
	this->addChild(scoreLabel);

	//添加碰撞监测
	auto contactListener = EventListenerPhysicsContact::create();
	contactListener->onContactBegin = CC_CALLBACK_1(GameScene::onContactBegin, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(contactListener, this);

	//添加触摸监听
	auto touchListener = EventListenerTouchOneByOne::create();
	touchListener->onTouchBegan = CC_CALLBACK_2(GameScene::onTouchBegan, this);
	touchListener->onTouchEnded = CC_CALLBACK_2(GameScene::onTouchEnded, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
	return true;
}
//设置层的物理世界
void GameScene::setPhysicWorld(PhysicsWorld *world)
{
	m_physicWorld = world;
}
//游戏开始
void GameScene::gameStart()
{
	gameStatus = GAME_START;
	score = 0;//重置分数
	scoreLabel->setString(String::createWithFormat("%d", score)->getCString());
	this->getChildByName("logo")->setVisible(false); //logo消失
	scoreLabel->setVisible(true); //计分开始
	this->scheduleUpdate();//启动默认更新
	this->schedule(schedule_selector(GameScene::scrollLand), 0.01f); //启动管子和地板滚动
	birdSprite->stopAction(swingAction); //游戏开始后停止上下浮动
	birdSprite->getPhysicsBody()->setGravityEnable(true); //开始受重力作用
}
//游戏结束
void GameScene::gameOver()
{
	gameStatus = GAME_OVER;
	//获取历史数据
	bestScore = UserDefault::getInstance()->getIntegerForKey("BEST");
	if (score > bestScore)
	{
		bestScore = score;  //更新最好分数
		UserDefault::getInstance()->setIntegerForKey("BEST", bestScore);
	}
		
	
	SimpleAudioEngine::getInstance()->playEffect("hit.mp3");
	//游戏结束后停止地板和管道的滚动
	this->unschedule(schedule_selector(GameScene::scrollLand));

	
}
//碰撞监测
bool GameScene::onContactBegin(const PhysicsContact& contact)
{
	if (gameStatus == GAME_OVER)  //当游戏结束后不再监控碰撞
		return false;
	
	gameOver();
	return true;
}
//触摸监听
bool GameScene::onTouchBegan(Touch *touch, Event *event)
{
	switch (gameStatus)
	{
	case GAME_OVER:
		break;
	case GAME_READY:
	{
		gameStart();
		birdSprite->getPhysicsBody()->setVelocity(Vec2(0, 250)); //给一个向上的初速度
		SimpleAudioEngine::getInstance()->playEffect("wing.mp3");
		//这里也要写，不然touchx的值是未知值，负无穷，导致bug
		touchX = touch->getLocation().x;
	}
		break;
	case GAME_START:
	{
		auto curVelocity = birdSprite->getPhysicsBody()->getVelocity();
		birdSprite->getPhysicsBody()->setVelocity(Vec2(0, 250>(curVelocity.y + 500) ? (curVelocity.y + 500):250)); //向上的速度受下降影响
		SimpleAudioEngine::getInstance()->playEffect("wing.mp3");
		//开上帝视角，留个后门，嘿嘿
		touchX = touch->getLocation().x;
	}
		break;
	default:
		break;
	}

	return true;
}
void GameScene::onTouchEnded(Touch *touch, Event *event)
{
	//当触摸点滑动超过100，分数瞬间涨100
	if (touch->getLocation().x - touchX > 100)
	{
		score += 100;
		scoreLabel->setString(String::createWithFormat("%d", score)->getCString());
		SimpleAudioEngine::getInstance()->playEffect("point.mp3");
	}
		
}
//地板滚动自定义计时器回调
void GameScene::scrollLand(float dt)
{
	Size visibleSize = Director::getInstance()->getVisibleSize();
	//两个图片循环移动
	land1->setPositionX(land1->getPositionX() - 1.0f);
	land2->setPositionX(land1->getPositionX() + land1->getContentSize().width - 2.0f);
	if (land2->getPositionX() <= 0)
		land1->setPosition(Point::ZERO);

	//管子滚动
	for (auto &singlePipe : pipes)
	{
		singlePipe->setPositionX(singlePipe->getPositionX() - 1.0f);
		if (singlePipe->getPositionX() < -PIPE_WIDTH/2)
		{
			singlePipe->setPositionX(visibleSize.width+PIPE_WIDTH/2);
			singlePipe->setPositionY(getRandomHeight());
			singlePipe->setName("newPipe");  //每次重设一根管子，标为new
		}
	}
}
//获取随机高度，用于管道
int GameScene::getRandomHeight()
{
	auto size = Director::getInstance()->getVisibleSize();
	//使得单根管子纵向坐标在屏幕中心点-(40~270)中间随机波动,这个是自己试出来的
	return size.height / 2 - 40 - CCRANDOM_0_1() * (270 - 40);
}
//创建管道 
void GameScene::createPipes()
{
	//同屏幕出现的只有两根管子，放到容器里面，上下绑定为一根
	for (int i = 0; i < 2; i++)
	{
		auto visibleSize = Director::getInstance()->getVisibleSize();
		Sprite *pipeUp = Sprite::createWithSpriteFrameName("pipe_up.png");
		Sprite *pipeDown = Sprite::createWithSpriteFrameName("pipe_down.png");
		Node *singlePipe = Node::create();
		//给上管绑定刚体
		auto pipeUpBody = PhysicsBody::createBox(pipeUp->getContentSize());
		pipeUpBody->setDynamic(false);
		pipeUpBody->setContactTestBitmask(1);
		pipeUp->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		pipeUp->setPhysicsBody(pipeUpBody);
		//给两个管子分开设置刚体，可以留出中间的空隙使得小鸟通过
		//给下管绑定刚体
		auto pipeDownBody = PhysicsBody::createBox(pipeDown->getContentSize());
		pipeDownBody->setDynamic(false);
		pipeDownBody->setContactTestBitmask(1);
		pipeDown->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		pipeDown->setPhysicsBody(pipeDownBody);

		pipeUp->setPosition(0, PIPE_HEIGHT + PIPE_SPACE);
		singlePipe->addChild(pipeUp);
		singlePipe->addChild(pipeDown);  //pipeDown默认加到（0,0），上下合并，此时singlePipe以下面的管子中心为锚点
		singlePipe->setPosition(i*PIPE_INTERVAL + WAIT_DISTANCE, getRandomHeight() ); //设置初始高度
		singlePipe->setName("newPipe");
		this->addChild(singlePipe);  //把两个管子都加入到层
		pipes.pushBack(singlePipe);  //两个管子先后添加到容器
	}

}
//默认的更新函数
void GameScene::update(float dt)
{
	//当游戏开始时，判断得分，这个其实也可以写在其他地方，比如管子滚动的更新函数里面或者触摸监测里面
	if (gameStatus == GAME_START)
	{
		for (auto &pipe : pipes)
		{
			if (pipe->getName() == "newPipe") //新来一根管子就判断
			{
				if (pipe->getPositionX() < birdSprite->getPositionX())
				{
					score++;
					scoreLabel->setString(String::createWithFormat("%d", score)->getCString());
					SimpleAudioEngine::getInstance()->playEffect("point.mp3");
					pipe->setName("passed"); //标记已经过掉的管子
				} 
			}
		}
	}
	//小鸟的旋转
	auto curVelocity = birdSprite->getPhysicsBody()->getVelocity();
	birdSprite->setRotation(-curVelocity.y*0.1 - 20);  //根据竖直方向的速度算出旋转角度，逆时针为负
	
	if (birdSprite->getPositionY() <= land1->getContentSize().height+BIRD_RADIUS)
	{
		birdSprite->stopAllActions(); //小鸟挂了就不能再扇翅了
		birdSprite->setRotation(70);  //设置为嘴朝下，顺时针70度旋转
		birdSprite->getPhysicsBody()->setDynamic(false);  //设置为不动了
		SimpleAudioEngine::getInstance()->playEffect("die.mp3");
		this->unscheduleUpdate();  //在小鸟掉到地面上再停止默认更新
		gamePanelAppear();  //弹出记分牌
	}
	
}
//加入记分板和重玩菜单
void GameScene::gamePanelAppear()
{
	Size size = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();
	//用node将gameoverlogo和记分板绑在一起
	Node *gameOverPanelNode = Node::create();
	auto gameOverLabel = Sprite::createWithSpriteFrameName("gameover.png");
	gameOverPanelNode->addChild(gameOverLabel);
	auto panel = Sprite::createWithSpriteFrameName("board.PNG");//注意这里是大写PNG，原图片用什么后缀这里就用什么，区分大小写
	gameOverLabel->setPositionY(panel->getContentSize().height); //设置一下坐标
	gameOverPanelNode->addChild(panel);
	//记分板上添加两个分数
	auto curScoreTTF = LabelTTF::create(String::createWithFormat("%d", score)->getCString(), "Arial", 20);
	curScoreTTF->setPosition(panel->getContentSize().width-40, panel->getContentSize().height-45);
	curScoreTTF->setColor(Color3B(255, 0, 0));
	panel->addChild(curScoreTTF);
	auto bestScoreTTF = LabelTTF::create(String::createWithFormat("%d", bestScore)->getCString(), "Arial", 20);
	bestScoreTTF->setPosition(panel->getContentSize().width - 40, panel->getContentSize().height - 90);
	bestScoreTTF->setColor(Color3B(0, 255, 0));
	panel->addChild(bestScoreTTF);
	this->addChild(gameOverPanelNode);
	gameOverPanelNode->setPosition(origin.x + size.width / 2, origin.y + size.height );
	//滑入动画
	gameOverPanelNode->runAction(MoveTo::create(0.5f, Vec2(origin.x + size.width / 2, origin.y + size.height / 2)));
	SimpleAudioEngine::getInstance()->playEffect("swooshing.mp3");
	//添加菜单
	MenuItemImage *restartItem = MenuItemImage::create("start_btn.png", "start_btn_pressed.png", this,menu_selector(GameScene::gameRetart));
	auto menu = CCMenu::createWithItem(restartItem);
	menu->setPosition(origin.x + size.width / 2, 150);
	this->addChild(menu);
}
//游戏重新开始
void GameScene::gameRetart(Ref *sender)
{
	//重新回到初始画面
	auto gameScene = GameScene::createScene();
	Director::getInstance()->replaceScene(gameScene); //这里懒得加特效了，直接转场
}