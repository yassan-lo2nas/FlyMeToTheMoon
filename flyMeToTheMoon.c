/*

  flyMeToTheMoon.c

  1.プログラムの内容
  自機を操作し、追いつかれないように右上の月まで逃げるゲーム。9回キー入力を行った時点で敵は
  一定距離で追尾をし始め、0.5秒放置することに1回移動分近づいてくる。
  ゲーム性を保持する程度に追いかけてくる敵を作るのが難しかった。

  2.起動・操作方法
  起動時点で操作可能。操作は
  ↑：上方向に移動
  →：右方向に移動
  ↓：下方向に移動
  ←：左方向に移動
  ESC：ゲームを強制終了(もう一度適当にキーを押して終了)
  また、ゴールするか、敵に追いつかれた時点でゲーム終了,何かのキーを押すとプログラム終了

  3.制限事項,不具合点
  明確な不具合点としては、敵の最初の挙動がおかしいこと。((0,0)と初期座標(S_x,S_y)を行き来している。ダブルバッファの仕様のため(?))
  制限事項としては、ステージが少なかったり、普通にやれば簡単であること。
  障害物などを設置するべきであった。壁の当たり判定が厳し目なのは仕様。
  また、マクロ(#define)をいじるだけでwindow幅等を変更出来るようにはなっていないので、
  500*500でしか楽しめないこと。
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <handy.h>
#include <time.h>
#define NUM 10   //記憶する座標数
#define WINDOW   500.0
#define INTERVAL 0.5
#define ESCKey   0x1B
#define S_x 25.0 //初期x
#define S_y 25.0 //初期y
#define TIME 1.8 //敵機のサイズ倍率

/*-- マクロ定義 --*/
typedef struct Me{//自機の情報を定義
  double x, y, size, dx, dy;
  int flag;//向きの判定(0~3, 0:上向き)
}Me;
struct Me me1 = {S_x, S_y, 10.0, 0.8 * 10.0, 0.8*10.0, 0};// {x, y, size, dx, dy, flag}

typedef struct D_wall{//触れたらGameOverとなる壁の情報を定義
  double x, y, wid, hei;
}D_wall;
struct D_wall dwall1 = {45.0, WINDOW/3.0, 100.0-45.0, 20.0};// 座標を与える{x, y, wid, hei}
struct D_wall dwall2 = { 5.0, WINDOW/3.0*2.0, 65.0, 20.0};// 座標を与える{x, y, wid, hei}
struct D_wall dwall3 = {400.0, WINDOW/2.0 + 40.0, 20.0, 10.0};// 座標を与える{x, y, wid, hei}
struct D_wall dwall4 = {400.0, WINDOW/2.0 - 50.0, 20.0, 10.0};// 座標を与える{x, y, wid, hei}
struct D_wall dwall5 = {150.0 , WINDOW/2.0 , 20.0, 20.0};// 座標を与える{x, y, wid, hei}
struct D_wall dwall6 = {220.0 , WINDOW/2.0 +50.0, 30.0, 20.0};// 座標を与える{x, y, wid, hei}
struct D_wall dwall7 = {270.0 , WINDOW/2.0 -50.0, 30.0, 20.0};// 座標を与える{x, y, wid, hei}
struct D_wall dwall8 = {300.0 , 400.0 , 20.0, 10.0};// 座標を与える{x, y, wid, hei}
/*---------------*/

void object(double size, int lid){//背景,物体を描画
  /* 背景色 */
  HgWSetColor(lid, HG_BLACK);
  HgWSetFillColor(lid, HG_BLACK);
  HgWBoxFill(lid, 0, 0, WINDOW, WINDOW, 0);
  /* ゴール */
  HgWSetColor(lid, HG_YELLOW);
  HgWSetFillColor(lid, HG_YELLOW);
  HgWCircleFill(lid, WINDOW-S_x+10.0, WINDOW-S_y+10.0, size, 0);//ゴール(月)
  HgWSetColor(lid, HG_BLACK);
  HgWSetFillColor(lid, HG_BLACK);
  HgWCircleFill(lid, WINDOW-S_x-(size/4.0)+10.0, WINDOW-S_y+(size/4.0)+10.0, size/1.4, 0);//影
  /* 壁 */
  HgWSetColor(lid, HG_BLUE);
  HgWSetFillColor(lid, HG_BLUE);

  HgWBoxFill(lid, 100, 0, size*2.0, WINDOW - 100, 0);
  HgWBoxFill(lid, 200, 100, size*2.0, WINDOW - 100, 0);
  HgWBoxFill(lid, 300, 0, 20, WINDOW - 100, 0);
  HgWBoxFill(lid, 400, 0, 20, WINDOW/2 - 50, 0);
  HgWBoxFill(lid, 400, WINDOW/2 + 50, size*2.0, WINDOW - 50, 0);

  HgWBoxFill(lid, 0, 0, WINDOW, 5, 0);//枠下
  HgWBoxFill(lid, 0, 0, 5, WINDOW, 0);//枠左
  HgWBoxFill(lid, 0, WINDOW-5, WINDOW, 5, 0);//枠上
  HgWBoxFill(lid, WINDOW-5, 0, 5, WINDOW, 0);//枠右

}
void pac(double x, double y, double size, int s, int lid, int flag)
{//自機,敵機を表示する関数
  switch(flag){
  case 0:
    if(s==0)HgWFanFill(lid, x, y, size, 0.75*M_PI, 2.25*M_PI, 0); //上向き
    else HgWFanFill(lid, x, y, size, 0.51*M_PI, 2.50*M_PI, 0); //上向き(口閉)
    break;
  case 1:
    if(s==0)HgWFanFill(lid, x, y, size, 0.25*M_PI, 1.75*M_PI, 0); //右向き
    else HgWFanFill(lid, x, y, size, 0.01*M_PI, 2.00*M_PI, 0); //右向き(口閉)
    break;
  case 2:
    if(s==0)HgWFanFill(lid, x, y, size, 1.75*M_PI, 3.25*M_PI, 0); //下向き
    else HgWFanFill(lid, x, y, size, 1.51*M_PI, 3.50*M_PI, 0); //下向き(口閉)
    break;
  case 3:
    if(s==0)HgWFanFill(lid, x, y, size, 1.25*M_PI, 0.75*M_PI, 0);//左向き
    else HgWFanFill(lid, x, y, size, 1.01*M_PI, 3.00*M_PI, 0);//左向き(口閉)
    break;
  }
}
void Danger(int lid, int t){//即死壁を描画する関数,点滅させるためにtを受け取る
  if(t % 2 == 0) {
    HgWSetColor(lid, HG_YELLOW); HgWSetFillColor(lid, HG_YELLOW);
  }else{
    HgWSetColor(lid, HG_RED); HgWSetFillColor(lid, HG_RED);
  }
  HgWBoxFill(lid, dwall1.x, dwall1.y, dwall1.wid, dwall1.hei, 0);
  HgWBoxFill(lid, dwall2.x, dwall2.y, dwall2.wid, dwall2.hei, 0);
  HgWBoxFill(lid, dwall3.x, dwall3.y, dwall3.wid, dwall3.hei, 0);
  HgWBoxFill(lid, dwall4.x, dwall4.y, dwall4.wid, dwall4.hei, 0);
  HgWBoxFill(lid, dwall5.x, dwall5.y, dwall5.wid, dwall5.hei, 0);
  HgWBoxFill(lid, dwall6.x, dwall6.y, dwall6.wid, dwall6.hei, 0);
  HgWBoxFill(lid, dwall7.x, dwall7.y, dwall7.wid, dwall7.hei, 0);
  HgWBoxFill(lid, dwall8.x, dwall8.y, dwall8.wid, dwall8.hei, 0);

}

void move (hgevent *ev){
  //自機の座標移動
  switch (ev->ch) {//キー操作が以下の場合
  case HG_U_ARROW:
    me1.flag = 0;
    me1.y += me1.dy;
    if(me1.y >= WINDOW - me1.size) me1.y = WINDOW - me1.size;//天井
    if(me1.x >= 200 && me1.x <= 200 + me1.size*2.0 && me1.y >=100-me1.size) me1.y=100-me1.size;//2枚目壁
    if(me1.x >= 400 && me1.x <= 400 + me1.size*2.0 && me1.y >=300-me1.size) me1.y=300-me1.size;//4枚目壁
    break;
  case HG_R_ARROW:
    me1.flag = 1;
    me1.x += me1.dx;
    if(me1.x >= WINDOW - me1.size) me1.x = WINDOW - me1.size;//右壁
    if(me1.x >= 100 -me1.size && me1.x <= 120 && me1.y <= 400+me1.size) me1.x = 100-me1.size;//1枚目壁
    if(me1.x >= 200 -me1.size && me1.x <= 220 && me1.y >= 100-me1.size) me1.x = 200-me1.size;//2枚目壁
    if(me1.x >= 300 -me1.size && me1.x <= 320 && me1.y <= 400+me1.size) me1.x = 300-me1.size;//3枚目壁
    if(me1.x >= 400 -me1.size && me1.x <= 420 && (me1.y <= 200 || me1.y >= 300)) me1.x = 400-me1.size;//4枚目壁
    break;
  case HG_D_ARROW:
    me1.flag = 2;
    me1.y -= me1.dy;
    if(me1.y <= me1.size) me1.y = me1.size;//床
    if(me1.x >= 100 && me1.x <= 120 && me1.y <= 400+me1.size) me1.y=400+me1.size;//1枚目壁
    if(me1.x >= 300 && me1.x <= 320 && me1.y <= 400+me1.size) me1.y=400+me1.size;//3枚目壁
    if(me1.x >= 400 && me1.x <= 420 && me1.y <= 200+me1.size) me1.y=200+me1.size;//4枚目壁
    break;
  case HG_L_ARROW:
    me1.flag = 3;
    me1.x -= me1.dx;
    if(me1.x <= me1.size) me1.x = me1.size;//左壁
    if(me1.x >= 100 && me1.x <= 120 +me1.size && me1.y <= 400+me1.size) me1.x = 120+me1.size;//1枚目壁
    if(me1.x >= 200 && me1.x <= 220 +me1.size && me1.y >= 100-me1.size) me1.x = 220+me1.size;//2枚目壁
    if(me1.x >= 300 && me1.x <= 320 +me1.size && me1.y <= 400+me1.size) me1.x = 320+me1.size;//3枚目壁
    if(me1.x >= 400 && me1.x <= 420 +me1.size && (me1.y <= 200 || me1.y >= 300)) me1.x = 420+me1.size;//4枚目壁
    break;
  default: break;
  }
}


int main(void) {
  int end = 0;//0:続く,1:game over, 2:game clear
  int s = 0;//口パクパクの状態
  int t = 0;//口パクパクを調整(5回に1度)
  int j = 0;
  /* 自機は構造体としてマクロ定義した */
  double x2[NUM]={0}; double y2[NUM]={0};
  double size = me1.size *TIME;//敵sizeは自機のTIME倍
  int flag[10]={0};//操作記憶する配列
  int count = 0;//操作回数(9回まで)を記憶するカウンタ変数

  int lid = 0;
  hgevent* ev;//イベント取得
  int wid = HgOpen(WINDOW, WINDOW);
  doubleLayer dlayers = HgWAddDoubleLayer(wid);
  int ch = 0;//キー入力に対応する整数値

  /* ベースレイヤ背景 */
  object(me1.size, lid); //ベースレイヤに背景作成

  /* ゲーム開始 */
  HgSetEventMask(HG_KEY_DOWN | HG_TIMER_FIRE);//キー入力, タイマ発火
  HgSetIntervalTimer(INTERVAL); // タイマの設定
  do {
    lid = HgLSwitch(&dlayers);
    HgLClear(lid);// 画面を(消去&描画)更新
    /* 自機 */
    HgWSetColor(lid, HG_RED); // 赤色
    HgWSetFillColor(lid, HG_RED);
    pac(me1.x, me1.y, me1.size, s, lid, me1.flag);//自機
    /* 敵機 */
    HgWSetColor(lid, HG_BLUE); // 青色
    HgWSetFillColor(lid, HG_BLUE);
    if(count == NUM-1) pac(x2[0], y2[0], size, s, lid, flag[0]); //敵機
    /* 即死壁 */
    Danger(lid, t);

    /* イベント取得 */
    ev = HgEventNonBlocking();
    if(ev != NULL){
      switch(ev->type){
      case HG_KEY_DOWN:
	if( (ch = ev->ch) != ESCKey){//ESC押す
	  move(ev);//方向キーに応じた計算
	  /* 敵機の方向,座標 */
	  flag[count]=me1.flag;
	  x2[count]=me1.x;
	  y2[count]=me1.y;
	  if(count < NUM-1) {
	    count++;//10回目入力まで記憶のみ
	  } else if(count==NUM-1 //9回入力以降
		    && x2[0]> me1.x-size && x2[0]< me1.x+size
		    && y2[0]> me1.y-size && y2[0]< me1.y+size){
	    end = 1; break;//追いつかれた
	  }
	  /* 即死壁に触れても終了 */
	  if(me1.x>= dwall1.x-me1.size            && me1.y>= dwall1.y+-me1.size &&
	     me1.x<= dwall1.x+dwall1.wid+me1.size && me1.y<= dwall1.y+dwall1.hei+me1.size){
	    end = 1; break;
	  }
	  if(me1.x>= dwall2.x-me1.size            && me1.y>= dwall2.y+-me1.size &&
	     me1.x<= dwall2.x+dwall2.wid+me1.size && me1.y<= dwall2.y+dwall2.hei+me1.size){
	    end = 1; break;
	    }
	  if(me1.x  >= dwall3.x- me1.size         && me1.y >= dwall3.y - me1.size&&
	     me1.x<= dwall3.x+dwall3.wid+me1.size && me1.y<= dwall3.y+dwall3.hei+me1.size){
	    end = 1; break;
	  }
	  if(me1.x  >= dwall4.x- me1.size         && me1.y >= dwall4.y - me1.size&&
	     me1.x<= dwall4.x+dwall4.wid+me1.size && me1.y<= dwall4.y+dwall4.hei+me1.size){
	    end = 1; break;
	  }
	  if(me1.x  >= dwall5.x- me1.size         && me1.y >= dwall5.y - me1.size&&
	     me1.x<= dwall5.x+dwall5.wid+me1.size && me1.y<= dwall5.y+dwall5.hei+me1.size){
	    end = 1; break;
	  }
	  if(me1.x  >= dwall6.x- me1.size         && me1.y >= dwall6.y - me1.size&&
	     me1.x<= dwall6.x+dwall6.wid+me1.size && me1.y<= dwall6.y+dwall6.hei+me1.size){
	    end = 1; break;
	  }
	  if(me1.x  >= dwall7.x- me1.size         && me1.y >= dwall7.y - me1.size&&
	     me1.x<= dwall7.x+dwall7.wid+me1.size && me1.y<= dwall7.y+dwall7.hei+me1.size){
	    end = 1; break;
	  }
	  if(me1.x  >= dwall8.x- me1.size         && me1.y >= dwall8.y - me1.size&&
	     me1.x<= dwall8.x+dwall8.wid+me1.size && me1.y<= dwall8.y+dwall8.hei+me1.size){
	    end = 1; break;
	  }
	  if(end == 1) break;//終了

	  /* ゴール判定 */
	  if(me1.x>= WINDOW-S_x-me1.size-4+10.0 && me1.y>= WINDOW-S_y-me1.size-4+10.0 &&
	     me1.x<= WINDOW-S_y+me1.size+4+10.0 && me1.y<= WINDOW-S_y+me1.size+4+10.0){
	    end = 2; break;//終了
	  }

	  /* [0]に[1]を代入し、以下同様に更新 */
	  for(j=0; j<NUM-1; j++){
	    flag[j]=flag[j+1];
	    x2[j]=x2[j+1];
	    y2[j]=y2[j+1];
	  }
	}
	break;

      case HG_TIMER_FIRE: //タイマ変数で、操作しない時,30f毎に追いかけてくる

	if(count==9){     //敵機が表示されている状態(9回入力後)
	  for(j=0; j<9; j++){
	    flag[j]=flag[j+1];
	    x2[j]=x2[j+1];
	    y2[j]=y2[j+1];
	  }
	  if(x2[0]> me1.x-size && x2[0]< me1.x+size &&
	     y2[0]> me1.y-size && y2[0]< me1.y+size){
	    end = 1;
	  }

	}

      break;
      }
    }
    /* ループ毎の処理 */
    if(end == 1 || end == 2) break;//終了フラグ立ってるならbreak
    if(t%10 == 0) s=(s+1)%2;//0と1の(口パク)状態移行を繰り返す
    t++;
    HgSleep(0.01); // 少し待つ;

  }while(ch != ESCKey);

  //ゲーム終了
  HgSetColor(HG_WHITE);
  HgSetFillColor(HG_WHITE);
  HgSetFont(HG_M, 28);

  if(end == 2) {
    printf("[Game Clear!]\n");
    HgText(WINDOW/3.0 -10.0 , WINDOW/2.0, "Game Clear!", 20);
  }
  else if(end == 1) {
    printf("[Game Over!]\n");
    HgText(WINDOW/3.0 -10.0 , WINDOW/2.0, "Game Over!", 20);
  }
  HgGetChar();
  HgClose();
  return 0;
}
