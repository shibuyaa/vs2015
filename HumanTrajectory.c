///////////////////////////////////////////////////////////////////////////////////////////////////////////
// 軌跡ファイルの表示 サンプルプログラム
// Masaki Onishi @ AIST 
//                                               ver 3.0 
// 独習のため必要最低限の構成になっています
// OepnGL 画面上で f ボタンを押すと image.ppm ファイルができます
// (1)今は人の位置に点を打っていますが人の位置に○を書くように変更してみよう
// (2)今は全ての点が赤ですがＩＤ毎に違う色になるように変更してみよう
// (3)今は現在の場所のみに点を打っていますが過去の軌跡をつないで動線として可視化してみよう
// (4)ファイル上のすべての軌跡を重ねて可視化してみよう
// (5)今はカメラ画像上の人の位置 x,y に点を打っていますがカメラ座標系の (x,y,z) の x, y に点を打ってみよう
// 　 ただし x = px*100/2+320 y = py*100/2+240 の値を使って下さい
// (6)似た軌跡は表示するのをやめて代表的な軌跡のみ可視化してみよう
// (7)滞留点を可視化してみよう
///////////////////////////////////////////////////////////////////////////////////////////////////////////

// ヘッダの読み込み
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES//M_PI=3.14..とかの定義を使う
#include <math.h>

#include <GL/glut.h>

// グローバル変数
unsigned char gimage[480][640][3];
unsigned char dataimage[128][128][3];//一人一人の進行方向を上とした128*128の画像

unsigned char outImage1[480][640][3];
unsigned char outImage2[480][640][3];//画像を一瞬コピペするやつ
unsigned char tmp1;

FILE *gfp;

#define MAXPOINTS 4500 /*記憶する点の数*/
#define MAXNO 20 /*記憶する人数*/
#define tai 3 /*滞留点の一人当たりの最大数*/
#define ms 4 /*何秒先を正解データとするか*/

//下の三つを調整する
GLdouble imr = 1.2; /*(128*imr)*(128*imr)の範囲を128*128の画像にする*/
GLdouble mx = 1.5; // 正解移動データの横幅
GLdouble my = 11; // 正解移動データの縦幅
GLint nstep = 20; //平均の速さを何フレームでとるか
				  /*
				  GLfloat translate[] = {
				  0 , 0 , 0 , 0 ,
				  0 , 0 , 0 , 0 ,
				  0 , 0 , 0 , 0 ,
				  0 , 0 , 0 , 1
				  };*/

GLint point[2600][MAXPOINTS][2] = { 0 }; /*座標を記憶する配列(2600はIDのmax),point[][][0]とpoint[][][1]はx,y座標*/
GLint point2[2600][MAXPOINTS][2] = { 0 }; /*pointを回転させたもの*/
GLdouble point3[2600][MAXPOINTS][2] = { 0 }; /*小数点まで含めた座標*/
GLint move[2600][MAXPOINTS] = { 0 };//どこに移動したかの正解
GLint moveSum[16] = { 0 }; //moveの数
GLint idno[ms][MAXNO] = { 0 }; /*1ステップで記憶するid(次の期でidが残ってるか調べるため)*/
							   //msフレーム前のidをidno[ms][]に記録

GLint stepnum[MAXPOINTS] = { 0 }; /*記憶した座標の数*/
GLint stepnumM[2600] = { 0 }; //idごとの今までのステップの最大値
GLint pointx[2600] = { 0 };/*今までに似た軌跡がある(1)かない(0)か(id毎)*/
						   //GLint stay[2600][tai][2] = { 0 };/*id毎に最大で5つまで滞留点(x,y)を記憶する*/
GLdouble v[2600][2] = { 0 };//各idの点の速度
GLdouble vm[2600][2] = { 0 };//各idの平均速度
GLint step[MAXPOINTS] = { 0 }; //stepnumM[id]と同じにする 平均速度を求めるために使う
							   //GLdouble sita[2600] = { 0 }; //idごとの進行方向と元の座標系のｙ軸との角度の差
							   //GLdouble cos_sita[2600] = { 0 }; //idごとのcosθ

							   //軌跡ファイルと画像ファイルを取得
							   //データのパスをチェックしてください
char csvfile[128] = "C:\\Users\\member\\Documents\\shibuya\\ConsoleApplication4\\data\\20140813-220002.csv"; //読み込む軌跡のファイル
char ppmfile[128] = "C:\\Users\\member\\Documents\\shibuya\\ConsoleApplication4\\data\\20140813-220002.ppm"; //読み込む画像のファイル

																											 //ファイル名を変数にする
char filename[64];


//回転させるためのsin,cosの式
/*
float rotsin(float x, float y) { //{(x2, y2) - (x1, y2)}のベクトルが上向きになるように回転させる行列のsinの値
float rs;
if (x == 0 && y == 0) {
return 0;
}
else{
rs = y / sqrtf(x*x + y*y);
return rs;
}
}

float rotcos(float x, float y) {
float rc;
if (x == 0 && y == 0) {
return 0;
}

else {
rc = x / sqrtf(x*x + y*y);
return rc;
}
}
*/

float sinc(float vx, float vy) { //画像の座標系でのvx,vyを使って表示の座標系での三角関数を求める
	float rs;
	if (vx == 0 && vy == 0) {
		return 0;
	}
	else if (vx == 0) {
		return 0;
	}
	else if (vy == 0) {
		return 1;
	}
	else {
		rs = vx / sqrtf(vx*vx + (16 / 9)*vy*vy);
		return rs;
	}
}

float cosc(float vx, float vy) {
	float rc;
	if (vx == 0 && vy == 0) {
		return -1;
	}
	else if (vx == 0) {
		return -1;
	}
	else if (vy == 0) {
		return 0;
	}
	else {
		rc = (-vy) / sqrtf((9 / 16)*vx*vx + vy*vy);
		return rc;
	}
}




/*
//ppm ファイルを書き込む関数（画像サイズは30×30のみ対応）
int ppm_write30(char *filename, unsigned char *pimage) {
FILE *fp;
fp = fopen(filename, "wb");
fprintf(fp, "P6\n30 30\n255\n");
fwrite(pimage, sizeof(char), 30 * 30 * 3, fp);
fclose(fp);
return 0;
}*/


void mouse(int button, int state, int x, int y) { //マウスボタン

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) printf("左ボタンが押されました"); //左ボタン
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)	  printf("左ボタンが離されました"); //左ボタン

}

void motion(int x, int y) { //マウスのクリック時

	printf(" (%d, %d) でマウスクリック！ \n", x, y);

}

void motion2(int x, int y) { //クリックしてない時のマウスの動き
							 //特になし
							 // printf("マウスの動き　%d,%d",x,y);
}

void keyboard(unsigned char key, int x, int y) { //キーボード

	switch (key) {

	case 'q': case 'Q': case '\033':  // ESC の ASCII コード
		exit(0);
	case 'f': //ファイルを画像として書き出す　glReadPixels　の仕様によりサイズが上下逆さまになってしまう
		unsigned char outImage[480][640][3];
		unsigned char tmp;
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, 640, 480, GL_RGB, GL_UNSIGNED_BYTE, &outImage[0][0][0]);
		for (int y = 0; y<480 / 2; y++) { //上下の反転
			for (int x = 0; x<640; x++) {
				for (int i = 0; i<3; i++) {
					tmp = outImage[y][x][i];
					outImage[y][x][i] = outImage[479 - y][x][i];
					outImage[479 - y][x][i] = tmp;
				}
			}
		}
		//画像をファイルを書き出す
		FILE *fpout;
		fpout = fopen("image.ppm", "wb");
		fprintf(fpout, "P6\n640 480\n255\n");
		fwrite(outImage, sizeof(char), 640 * 480 * 3, fpout);
		fclose(fpout);
		break;

	default:
		break;
	}
}


void display(void) { //ここがループで回る

	static int counter = 0;

	int i = 0;

	int year, month, day, hour, min, sec, msec;
	int frame, no;
	int no2 = 20;//一期前の人数
				 //char str[64];

	int id, x, y;
	double x3, y3;
	float px, py, pz;
	int n = 200;//円の点の数
	double r = 0.03;//円の半径
	int j = 0;//角度θを作る,記録したデータを描画する
	int k = 0;//記録したデータを描画する2
	int l = 0;//繰り返しや判別に使う(似た軌跡)
	int m = 0;//繰り返しに使う
	int h = 0;//判別に使う
	double sum = 0.0;//似てる尺度
	int Mid = 0;//Maxid そのフレームの中の最大のid
	double vx[5] = { 0 };
	double vy[5] = { 0 };
	int tx, ty;//tmpx,tmpy
	int tx2, ty2;
	double jx, jy;//判定に使う

	float ox, oy, oz; //現在のところ値は入っていないので使わない
	char NS, EW; //現在のところ値は入っていないので使わない
	float wx, wy, wz; //現在のところ値は入っていないので使わない
	char subject[64], feature[64], gender[64], attribute[64], group[64], action[64], state[64], free[64]; //現在のところ値は入っていないので使わない

	char str[64];


	// 画像の描画
	glClear(GL_COLOR_BUFFER_BIT);

	glRasterPos2f(-1, 1); //左上
						  //glDrawPixels(640, 480, GL_RGB, GL_UNSIGNED_BYTE, &gimage[0][0][0]);//背景画像を描画する

	glPointSize(2); //2の大きさで点を打つ
	glLineWidth(2); //2の大きさの線
					//glBegin(GL_POINTS);
					//glColor3ub(255, 0, 0);

					//ファイルを読む
	if ((fscanf(gfp, "%04d:%02d:%02d-%02d:%02d:%02d:%03d,%d,%d,,,,,,,,,,,,,,,,,,,,,,\n", &year, &month, &day, &hour, &min, &sec, &msec, &frame, &no)) == EOF) {
		printf("ファイルが終了しました\n");
		system("pause"); exit(0);
	}
	printf("[%07d] %d/%02d/%02d %02d:%02d:%02d:%03d\n", counter, year, month, day, hour, min, sec, msec);




	for (i = 0; i < no; i++) {

		fscanf(gfp, ",,,%d,%d,%d,%f,%f,%f,,,,,,,,,%s\n", &id, &x, &y, &px, &py, &pz, &str[0]); //現在のところ値が入っていないところは省略する

																							   //printf("(%3d,%3d) ", x, y);


																							   //x = px * 220 + 320;//縮尺1.45と仮定(1.5なら160.212, 1.4なら171.257)
																							   //y = py * 166 + 240;

		x = px * 300 / 2 + 320;
		y = py * 300 / 2 + 240;

		x3 = px * 300 / 2 + 320;
		y3 = py * 300 / 2 + 240;
		//x = -2.0*px / 3.0;
		//y = -2.0*py / 3.0;


		printf("(%3d,%3d) ", x, y);


		/*
		if (id == NULL){
		id = 2550;
		}*/


		if (Mid < id) {//そのフレームのidのmaxを記録
			Mid = id;
		}


		//id毎の速度を記憶
		if (stepnum[i] > 4) {
			vx[0] = (point[id][stepnum[i] - 1][0] - point[id][stepnum[i] - 2][0]);
			vx[1] = (point[id][stepnum[i] - 2][0] - point[id][stepnum[i] - 3][0]);
			vx[2] = (point[id][stepnum[i] - 3][0] - point[id][stepnum[i] - 4][0]);
			vx[3] = (point[id][stepnum[i] - 4][0] - point[id][stepnum[i] - 5][0]);
			//vx[4] = (point[id][stepnum[i] - 5][0] - point[id][stepnum[i] - 6][0]);
			vy[0] = -(point[id][stepnum[i] - 1][1] - point[id][stepnum[i] - 2][1]);
			vy[1] = -(point[id][stepnum[i] - 2][1] - point[id][stepnum[i] - 3][1]);
			vy[2] = -(point[id][stepnum[i] - 3][1] - point[id][stepnum[i] - 4][1]);
			vy[3] = -(point[id][stepnum[i] - 4][1] - point[id][stepnum[i] - 5][1]);
			//vy[4] = -(point[id][stepnum[i] - 4][1] - point[id][stepnum[i] - 5][1]);
			v[id][0] = (vx[0] + vx[1] + vx[2] + vx[3]) / 4;
			v[id][1] = (vy[0] + vy[1] + vy[2] + vy[3]) / 4;
		}
		else if (stepnum[i] > 3) {
			vx[0] = (point[id][stepnum[i] - 1][0] - point[id][stepnum[i] - 2][0]);
			vx[1] = (point[id][stepnum[i] - 2][0] - point[id][stepnum[i] - 3][0]);
			vx[2] = (point[id][stepnum[i] - 3][0] - point[id][stepnum[i] - 4][0]);
			vy[0] = -(point[id][stepnum[i] - 1][1] - point[id][stepnum[i] - 2][1]);
			vy[1] = -(point[id][stepnum[i] - 2][1] - point[id][stepnum[i] - 3][1]);
			vy[2] = -(point[id][stepnum[i] - 3][1] - point[id][stepnum[i] - 4][1]);
			v[id][0] = (vx[0] + vx[1] + vx[2]) / 3;
			v[id][1] = (vy[0] + vy[1] + vy[2]) / 3;
		}
		else if (stepnum[i] > 2) {
			vx[0] = (point[id][stepnum[i] - 1][0] - point[id][stepnum[i] - 2][0]);
			vx[1] = (point[id][stepnum[i] - 2][0] - point[id][stepnum[i] - 3][0]);
			vy[0] = -(point[id][stepnum[i] - 1][1] - point[id][stepnum[i] - 2][1]);
			vy[1] = -(point[id][stepnum[i] - 2][1] - point[id][stepnum[i] - 3][1]);
			v[id][0] = (vx[0] + vx[1]) / 2;
			v[id][1] = (vy[0] + vy[1]) / 2;
		}
		else {
			v[id][0] = 0;//x方向の速度
			v[id][1] = 0;//y方向の速度
		}


		/*
		//idごとに角度を計算
		if (v[id][0] == 0 && v[id][1] == 0) {
		sita[id] = 0;
		}
		else {
		cos_sita[id] = (double)v[id][1] / sqrtf(v[id][0] * v[id][0] + v[id][1] * v[id][1]);
		sita[id] = acos(cos_sita[id]);
		sita[id] = sita[id] * 180 / M_PI; //180度表記
		}*/


		if (idno[1][i] > 0 && idno[1][i] == id) {//1期前と同じ場所に同じidだったら
												 //point[id][stepnum[i]][0] = x;//各idのpointnum[i]番目のx座標
												 //point[id][stepnum[i]][1] = y;
			point[id][step[id]][0] = x;
			point[id][step[id]][1] = y;
			point3[id][step[id]][0] = x3;
			point3[id][step[id]][1] = y3;
			stepnum[i]++;
		}
		else {//一期前と同じ場所に同じidがなかったら
			h = 0;
			for (j = 0; j < no2; ++j) {//一期前のid[j](jで回す)にid(今読み取ったid)がないか探す．
				if (id == 0) {
					break;
				}
				else if (idno[1][j] == id) {
					h = j + 1;//j+1行目に発見(したの>0判別のために+1)
					break;
				}
			}
			if (h > 0) {//もしみつかったら
				idno[1][i] = idno[1][h - 1];//idno[1][i]にidno[1][h-1](=id)をうつす
				stepnum[i] = stepnum[h - 1];//pointnum[i]をpointnum[h-1]で書き換える
											//point[id][stepnum[i]][0] = x;//各idのpointnum[i]番目のx座標
											//point[id][stepnum[i]][1] = y;
				point[id][step[id]][0] = x;
				point[id][step[id]][1] = y;
				point3[id][step[id]][0] = x3;
				point3[id][step[id]][1] = y3;
				stepnum[i]++;
				stepnum[h - 1] = 0;
			}
			else {//見つからなければ初期化
				stepnum[i] = 0;
			}
		}

		if ((id > 0) && (point[id][step[id]][0] == 0)) {
			//if ((id > 0) && (point[id][stepnum[i] - 1][0] == 0)) {
			//point[id][stepnum[i] - 1][0] = x;
			//point[id][stepnum[i] - 1][1] = y;
			point[id][step[id]][0] = x;
			point[id][step[id]][1] = y;
			point3[id][step[id]][0] = x3;
			point3[id][step[id]][1] = y3;

		}


		no2 = no;//一期前の人数を記録
		idno[0][i] = id;

		//idの最大ステップ数を記録
		if (id > 0) {
			stepnumM[id] = stepnum[i] + 1;
		}
		else {
			stepnumM[id] = 0;
		}

		//printf(" id (速度v,ステップ) = '%2d'((%2f,%2f),%2d) \n", id, v[id][0], v[id][1], stepnumM[id]);

		/*
		//平均の速度の計算
		if (step[id] == 0) {
		tx = 0;
		ty = 0;
		}
		else {
		//tx = point[id][stepnum[i] - 1][0] - point[id][stepnum[i] - 2][0];
		//ty = -(point[id][stepnum[i] - 1][1] - point[id][stepnum[i] - 2][1]);
		tx = point[id][step[id]][0] - point[id][step[id] - 1][0];
		ty = -(point[id][step[id]][1] - point[id][step[id] - 1][1]);
		//printf("(step,stepnum) (%d, %d) \n",stepnum[i] ,step[id]);
		vm[id][0] = (vm[id][0] * (step[id] - 1) + tx) / step[id];
		vm[id][1] = (vm[id][1] * (step[id] - 1) + ty) / step[id];
		}
		step[id] = step[id] + 1;
		*/
		/*
		//ステップ数２の平均
		if (step[id] == 0) {
		tx = 0;
		ty = 0;
		}
		else if(step[id] == 1){
		vm[id][0] = point[id][step[id]][0] - point[id][step[id] - 1][0];
		vm[id][1] = -(point[id][step[id]][1] - point[id][step[id] - 1][1]);

		}
		else {
		//tx = point[id][stepnum[i] - 1][0] - point[id][stepnum[i] - 2][0];
		//ty = -(point[id][stepnum[i] - 1][1] - point[id][stepnum[i] - 2][1]);
		tx = (point[id][step[id]][0] - point[id][step[id] - 2][0]) / 2;
		ty = -(point[id][step[id]][1] - point[id][step[id] - 2][1]) / 2;
		vm[id][0] = (vm[id][0] * (step[id] - 2) + tx) / (step[id] - 1);
		vm[id][1] = (vm[id][1] * (step[id] - 2) + ty) / (step[id] - 1);
		}
		step[id] = step[id] + 1;
		*/

		/*
		//ステップ数5の平均
		if (step[id] == 0) {
		tx = 0;
		ty = 0;
		}
		else if (step[id] == 1) {
		vm[id][0] = point[id][step[id]][0] - point[id][step[id] - 1][0];
		vm[id][1] = -(point[id][step[id]][1] - point[id][step[id] - 1][1]);

		}
		else if (step[id] == 2) {
		vm[id][0] = (point[id][step[id]][0] - point[id][step[id] - 2][0]) / 2;
		vm[id][1] = -(point[id][step[id]][1] - point[id][step[id] - 2][1]) / 2;
		}
		else if (step[id] == 3) {
		vm[id][0] = (point[id][step[id]][0] - point[id][step[id] - 3][0]) / 3;
		vm[id][1] = -(point[id][step[id]][1] - point[id][step[id] - 3][1]) / 3;

		}
		else if (step[id] == 4) {
		vm[id][0] = (point[id][step[id]][0] - point[id][step[id] - 4][0]) / 4;
		vm[id][1] = -(point[id][step[id]][1] - point[id][step[id] - 4][1]) / 4;
		}
		else {
		//tx = point[id][stepnum[i] - 1][0] - point[id][stepnum[i] - 2][0];
		//ty = -(point[id][stepnum[i] - 1][1] - point[id][stepnum[i] - 2][1]);
		tx = (point[id][step[id]][0] - point[id][step[id] - 5][0]) / 5;
		ty = -(point[id][step[id]][1] - point[id][step[id] - 5][1]) / 5;
		vm[id][0] = (vm[id][0] * (step[id] - 5) + tx) / (step[id] - 4);
		vm[id][1] = (vm[id][1] * (step[id] - 5) + ty) / (step[id] - 4);
		}
		step[id] = step[id] + 1;
		*/



		//平均の速さをnstepフレームの間隔で測定
		if (step[id] == 0) {
			tx = 0;
			ty = 0;
		}
		else if (step[id] < nstep) {
			vm[id][0] = (point[id][step[id]][0] - point[id][0][0]) / step[id];
			vm[id][1] = -(point[id][step[id]][1] - point[id][0][1]) / step[id];
		}
		else {
			//tx = point[id][stepnum[i] - 1][0] - point[id][stepnum[i] - 2][0];
			//ty = -(point[id][stepnum[i] - 1][1] - point[id][stepnum[i] - 2][1]);
			tx = (point[id][step[id]][0] - point[id][step[id] - nstep][0]) / nstep;
			ty = -(point[id][step[id]][1] - point[id][step[id] - nstep][1]) / nstep;
			vm[id][0] = (vm[id][0] * (step[id] - nstep) + tx) / (step[id] - (nstep - 1));
			vm[id][1] = (vm[id][1] * (step[id] - nstep) + ty) / (step[id] - (nstep - 1));
		}
		step[id] = step[id] + 1;





		//5ステップ後にどこに動いたかで番号をつける
		if (stepnumM[id]/*stepnum[i]でもいい*/ > (ms - 1)) {
			jx = point3[id][stepnumM[id] - 1][0] - point3[id][stepnumM[id] - ms - 1][0];
			jy = -(point3[id][stepnumM[id] - 1][1] - point3[id][stepnumM[id] - ms - 1][1]);

			if ((2 * my <= jy) && (jy < 3 * my) && (-5 * mx <= jx) && (jx < -3 * mx)) {
				move[id][stepnumM[id] - ms - 1] = 1;
			}
			else if ((2 * my <= jy) && (jy < 3 * my) && (-3 * mx <= jx) && (jx < -mx)) {
				move[id][stepnumM[id] - ms - 1] = 2;
			}
			else if ((2 * my <= jy) && (jy < 3 * my) && (-mx <= jx) && (jx <= mx)) {
				move[id][stepnumM[id] - ms - 1] = 3;
			}
			else if ((2 * my <= jy) && (jy < 3 * my) && (mx < jx) && (jx <= 3 * mx)) {
				move[id][stepnumM[id] - ms - 1] = 4;
			}
			else if ((2 * my <= jy) && (jy < 3 * my) && (3 * mx < jx) && (jx <= 5 * mx)) {
				move[id][stepnumM[id] - ms - 1] = 5;
			}
			else if ((my <= jy) && (jy < 2 * my) && (-5 * mx <= jx) && (jx < -3 * mx)) {
				move[id][stepnumM[id] - ms - 1] = 6;
			}
			else if ((my <= jy) && (jy < 2 * my) && (-3 * mx <= jx) && (jx < -mx)) {
				move[id][stepnumM[id] - ms - 1] = 7;
			}
			else if ((my <= jy) && (jy < 2 * my) && (-mx <= jx) && (jx <= mx)) {
				move[id][stepnumM[id] - ms - 1] = 8;
			}
			else if ((my <= jy) && (jy < 2 * my) && (mx < jx) && (jx <= 3 * mx)) {
				move[id][stepnumM[id] - ms - 1] = 9;
			}
			else if ((my <= jy) && (jy < 2 * my) && (3 * mx < jx) && (jx <= 5 * mx)) {
				move[id][stepnumM[id] - ms - 1] = 10;
			}
			else if ((0 <= jy) && (jy < my) && (-5 * mx <= jx) && (jx < -3 * mx)) {
				move[id][stepnumM[id] - ms - 1] = 11;
			}
			else if ((0 <= jy) && (jy < my) && (-3 * mx <= jx) && (jx < -mx)) {
				move[id][stepnumM[id] - ms - 1] = 12;
			}
			else if ((0 <= jy) && (jy < my) && (-mx <= jx) && (jx <= mx)) {
				move[id][stepnumM[id] - ms - 1] = 13;
			}
			else if ((0 <= jy) && (jy < my) && (mx < jx) && (jx <= 3 * mx)) {
				move[id][stepnumM[id] - ms - 1] = 14;
			}
			else if ((0 <= jy) && (jy < my) && (3 * mx < jx) && (jx <= 5 * mx)) {
				move[id][stepnumM[id] - ms - 1] = 15;
			}
			else {
				move[id][stepnumM[id] - ms - 1] = 16;
			}
		}

		if (move[id][stepnumM[id] - ms - 1] != 0) {
			printf("move %d\n", move[id][stepnumM[id] - ms - 1]);
			moveSum[move[id][stepnumM[id] - ms - 1] - 1] = moveSum[move[id][stepnumM[id] - ms - 1] - 1] + 1;
		}
		if (stepnumM[id] > ms - 1) {
			printf("移動(x,y) = (%d,d id(%d)\n", jx, jy, id);
		}

		/*
		//頭の〇を描く
		printf("x, pointx(%d,%d)\n", x, point[id][stepnum[i]-1][0]);
		if (id > 0) {
		glBegin(GL_POINTS);
		glColor3ub(0, 0, 0);
		//画像は左上が(0,0) 右下が(639,479)
		//表示の座標系は左上が (-1,1)，中心が (0,0) ，右下が(1,-1)だから座標変換して表示する
		for (j = 0; j < n; j++) { //nの数だけ(x,y)の周りに円状に点を打つ
		glVertex2d(((point[id][stepnum[i] - 1][0] - 320) / 320.0) + 0.75 * r * cos(2.0 * 3.14 * (double)j / n), (-(y - 240) / 240.0) + r * sin(2.0 * 3.14 * (double)j / n));//点を打つ
		}
		glEnd();
		}
		//記録したデータを線で描く
		if (stepnum[i] > 1) {
		//glLineWidth(2);
		for (j = 1; j < stepnum[i] + 1; ++j) {
		if (point[id][j][0] == 0 && point[id][j][1] == 0) break;
		else {
		glBegin(GL_LINES);
		glVertex2d((point[id][j - 1][0] - 320) / 320.0, (-(point[id][j - 1][1] - 240) / 240.0));
		glVertex2d((point[id][j][0] - 320) / 320.0, (-(point[id][j][1] - 240) / 240.0)); //2次元の座標値を設定
		glEnd();
		}
		}
		}*/

		/*
		//滞留点を記憶させる(y方向の移動(|y2-y1|^2 + |y3-y1|^2)が少ないものを滞留とした)
		for (j = 1; j < MAXPOINTS; ++j) {
		if (point[id][j][0] == 0 || point[id][j][1] == 0 || point[id][j + 1][0] == 0 || point[id][j + 1][1] == 0) break;
		else if ((point[id][j][1] - point[id][j - 1][1])*(point[id][j][1] - point[id][j - 1][1]) + (point[id][j + 1][1] - point[id][j - 1][1])*(point[id][j + 1][1] - point[id][j - 1][1]) < 2) {//(y2-y1)^2
		k = j % tai;
		stay[id][k][0] = (int)((point[id][j - 1][0] + point[id][j][0]) / 2); //j/5のあまりのところに二点の中点を代入
		stay[id][k][1] = (int)((point[id][j - 1][1] + point[id][j][1]) / 2);
		}
		}
		*/

		//px,pyを変えて正解データをつくる．
		/*
		if (stepnum[i] > 1) {
		//glLineWidth(2);
		for (j = 1; j < stepnum[i]; ++j) {
		if (point[id][j][0] == 0 && point[id][j][1] == 0) break;
		else {
		glBegin(GL_LINES);
		glVertex2d((point[id][j - 1][0] - 320) / 320.0, (-(point[id][j - 1][1] - 240) / 240.0));
		glVertex2d((point[id][j][0] - 320) / 320.0, (-(point[id][j][1] - 240) / 240.0)); //2次元の座標値を設定
		glEnd();
		}
		}
		}*/

		/*
		//正解データをつくる
		if ((145 < x < 495) && (65 < y < 415)) {//xは±225+320から端30,yは±225+240から端30の領域を消す
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, 640, 480, GL_RGB, GL_UNSIGNED_BYTE, &outImage1[0][0][0]);
		for (j = 0; j < 480 / 2; j++) { //上下の反転
		for (k = 0; k < 640; k++) {
		for (l = 0; l < 3; l++) {
		tmp1 = outImage1[j][k][l];
		outImage1[j][k][l] = outImage1[479 - j][k][l];
		outImage1[479 - j][k][l] = tmp1;
		}
		}
		}
		for (j = y - 50; j < y + 50; j++) {//点の近くの画像を収納（回転させるもの）
		for (k = x - 50; k < x + 50; k++) {
		for (l = 0; l < 3; l++) {
		dataimage[j-y+50][k-x+50][l] = outImage1[j][k][l];
		}
		}
		}
		*/




	}
	//printf("\n");
	//ここまでが１フレームのなかで起こっていることで以下がフレームごとにやること

	//そのフレームの中のidごとに，一つのidを赤それ以外を白で軌跡を描き，回転させてppmをつくる．
	printf("id(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\n", idno[1][0], idno[1][1], idno[1][2], idno[1][3], idno[1][4], idno[1][5], idno[1][6], idno[1][7], idno[1][8], idno[1][9], idno[1][10]);
	//printf("move(  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 )[%.2f,%.2f,%d]\n", mx, my, ms);
	printf("move(   1    2    3    4    5 )[%.2f,%.2f,%d]\n", mx, my, ms);
	//printf("    (%03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d )\n", moveSum[0], moveSum[1], moveSum[2], moveSum[3], moveSum[4], moveSum[5], moveSum[6], moveSum[7], moveSum[8], moveSum[9], moveSum[10], moveSum[11], moveSum[12], moveSum[13], moveSum[14], moveSum[15]);
	printf("    (%05d %05d %05d %05d %05d )\n", moveSum[0], moveSum[1], moveSum[2], moveSum[3], moveSum[4]);
	printf("    (%05d %05d %05d %05d %05d )\n", moveSum[5], moveSum[6], moveSum[7], moveSum[8], moveSum[9]);
	printf("    (%05d %05d %05d %05d %05d ) (%05d)\n", moveSum[10], moveSum[11], moveSum[12], moveSum[13], moveSum[14], moveSum[15]);

	//printf("mx my ms [%.2f,%.2f,%d]\n", mx, my, ms);
	//printf("sita(%f %f %f %f %f %f %f %f %f %f %f)\n", sita[idno[0][0]], sita[idno[0][1]], sita[idno[0][2]], sita[idno[0][3]], sita[idno[0][4]], sita[idno[0][5]], sita[idno[0][6]], sita[idno[0][7]], sita[idno[0][8]], sita[idno[0][9]], sita[idno[0][10]]);
	//printf("cos(sita)(%f %f %f %f %f)\n", cos_sita[idno[0][0]], cos_sita[idno[0][1]], cos_sita[idno[0][2]], cos_sita[idno[0][3]], cos_sita[idno[0][4]]);
	//printf("rotsin(%f %f %f %f %f)\n", rotsin(v[idno[0][0]][0], v[idno[0][0]][1]), rotsin(v[idno[0][1]][0], v[idno[0][1]][1]), rotsin(v[idno[0][2]][0], v[idno[0][2]][1]), rotsin(v[idno[0][3]][0], v[idno[0][3]][1]), rotsin(v[idno[0][4]][0], v[idno[0][4]][1]) );
	//printf("id-1(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\n", idno[1][0], idno[1][1], idno[1][2], idno[1][3], idno[1][4], idno[1][5], idno[1][6], idno[1][7], idno[1][8], idno[1][9], idno[1][10]);

	printf("\n");


	if (id > 0) {
		for (i = 0; i < 1; i++) { //idの速度の方向に同フレームの全てのidの軌跡(x,y)を回転させる(idの数だけ速度の方向が違うので繰り返す)
			if (idno[0][i] == 0) {
				break;
			}
			else {
				glClear(GL_COLOR_BUFFER_BIT);
				for (j = 0; j < 1; j++) {
					if (idno[0][i] == idno[0][j]) { //注目するidはidno[0][i]で赤
						glColor3ub(255, 0, 0);
					}
					else { //それ以外は黒
						glColor3ub(0, 0, 0);
					}
					if (idno[0][j] == 0) {
						break;
					}
					else {

						for (k = 0; k < stepnumM[idno[0][j]] - 1; k++) { //kはidno[0][j]のステップ を回転
																		 //printf("point(j,k) %3d(%d,%d)\n", point[idno[0][j]][k][0],idno[0][j],k);
							tx = point[idno[0][j]][k][0];
							ty = point[idno[0][j]][k][1];
							point2[idno[0][j]][k][0] = (int)(cosc(v[idno[0][i]][0], v[idno[0][i]][1])*tx - sinc(v[idno[0][i]][0], v[idno[0][i]][1])*ty * 320 / 240 + 320 * (1 - cosc(v[idno[0][i]][0], v[idno[0][i]][1]) + sinc(v[idno[0][i]][0], v[idno[0][i]][1])));//Acd*R(θ)*Adc *(x,y)^t(転置) のx座標
							point2[idno[0][j]][k][1] = (int)(sinc(v[idno[0][i]][0], v[idno[0][i]][1])*tx * 240 / 320 + cosc(v[idno[0][i]][0], v[idno[0][i]][1])*ty + 240 * (1 - cosc(v[idno[0][i]][0], v[idno[0][i]][1]) - sinc(v[idno[0][i]][0], v[idno[0][i]][1])));
							//printf("point[%d][%d](%d,%d)\n ",idno[0][j] , k, point2[idno[0][j]][k][0], point2[idno[0][j]][k][1]);

						}
						for (k = stepnumM[idno[0][j]] - 1; k < MAXPOINTS; k++) {
							point2[idno[0][j]][k][0] = 0;
							point2[idno[0][j]][k][1] = 0;
						}
						glBegin(GL_POINTS);
						//画像は左上が(0,0) 右下が(639,479)
						//表示の座標系は左上が (-1,1)，中心が (0,0) ，右下が(1,-1)だから座標変換して表示する
						for (k = 0; k < n; k++) { //nの数だけ(x,y)の周りに円状に点を打つ
												  //glVertex2d((point2[idno[0][j]][stepnumM[idno[0][j]] - 2][0] + 0.75 * r * cos(2.0 * 3.14 * (double)k / n)), (-point2[idno[0][j]][stepnumM[idno[0][j]] - 2][1] + r * sin(2.0 * 3.14 * (double)k / n)));//点を打つ							}
							glVertex2d((4 / 3)*imr*(((point2[idno[0][j]][stepnumM[idno[0][j]] - 2][0] - 320) / 320.0) + 0.75 * r * cos(2.0 * 3.14 * (double)k / n)/*/imr*/), imr*(((point2[idno[0][j]][stepnumM[idno[0][j]] - 2][1] - 240) / 240.0) + r * sin(2.0 * 3.14 * (double)k / n)/*/imr*/));//点を打つ
						}
						glEnd();
						if (stepnumM[idno[0][j]] > 1) {
							//glLineWidth(2);
							for (k = 1; k < stepnumM[idno[0][j]] - 2; ++k) {
								if (point[idno[0][j]][k][0] == 0 && point[idno[0][j]][k][1] == 0) break;
								else {
									glBegin(GL_LINES);
									//glVertex2d(point2[idno[0][j]][k - 1][0], -point2[idno[0][j]][k - 1][1]);
									//glVertex2d(point2[idno[0][j]][k][0], -point2[idno[0][j]][k][1]);
									tx = point2[idno[0][j]][k - 1][0];
									ty = point2[idno[0][j]][k - 1][1];
									tx2 = point2[idno[0][j]][k][0];
									ty2 = point2[idno[0][j]][k][1];
									double c;
									if (stepnumM[idno[0][j]] - 2 - k > 8) c = 8;
									else c = stepnumM[idno[0][j]] - 2 - k;

									for (l = 0; l < 5; l++) {//色を薄くする

										if (idno[0][i] == idno[0][j]) {
											if (c == 8) glColor3ub(255, 250, 250);
											else glColor3ub(255, c * 28 + l * 7, c * 28 + l * 7);
										}
										else {
											if (c == 8) glColor3ub(250, 250, 250);
											else glColor3ub(c * 28 + l * 7, c * 28 + l * 7, c * 28 + l * 7);
										}
										glVertex2d((4 / 3)*imr*(((4 - l) / 5.0)*tx2 + ((l + 1) / 5.0)*tx - 320) / 320.0, imr*(((4 - l) / 5.0)*ty2 + ((l + 1) / 5.0)*ty - 240) / 240.0);
										glVertex2d((4 / 3)*imr*(((5 - l) / 5.0)*tx2 + (l / 5.0)*tx - 320) / 320.0, imr*(((5 - l) / 5.0)*ty2 + (l / 5.0)*ty - 240) / 240.0); //2次元の座標値を設定

									}

									glEnd();
								}
							}
						}
					}
				}

				if ((145 < point2[idno[0][i]][stepnumM[idno[0][i]] - 2][0] < 495) && (65 < (-point2[idno[0][i]][stepnumM[idno[0][i] - 2]][1] + 480) < 415)) {//xは±225+320から端30,yは±225+240から端30の領域を消す
																																							 //注目する人の周りだけ切り取る
					glPixelStorei(GL_PACK_ALIGNMENT, 1);
					glReadPixels(0, 0, 640, 480, GL_RGB, GL_UNSIGNED_BYTE, &outImage1[0][0][0]);
					for (m = 0; m < 480 / 2; m++) { //上下の反転
						for (k = 0; k < 640; k++) {
							for (l = 0; l < 3; l++) {
								tmp1 = outImage1[m][k][l];
								outImage1[m][k][l] = outImage1[479 - m][k][l];
								outImage1[479 - m][k][l] = tmp1;
							}
						}
					}
					/*
					tx = (4 / 3)*imr*(point2[idno[0][i]][stepnumM[idno[0][i]] - 2][0]) + (1 - (4 / 3)*imr) * 320;
					ty = -imr*point2[idno[0][i]][stepnumM[idno[0][i]] - 2][1] + (1 + imr) * 240;
					//printf("ステップ(%d), ", stepnumM[idno[0][i]]);
					//printf("xy(%d,%d)\n", tx, ty);
					for (m = 0; m < 128; m++) { //y
						for (k = 0; k < 128; k++) { //x
							for (l = 0; l < 3; l++) {
								dataimage[m][k][l] = outImage1[ty - 110 + m][tx - 64 + k][l];//(idno[0][i]の最新の軌跡の周りだけ切り取る)
							}
						}
					}

					sprintf(filename, "image_%.2f\\imagev\\image3.ppm", imr);
					//sprintf(filename, "image_%.2f\\imagev\\image%d.%d.ppm", imr, idno[0][i], stepnumM[idno[0][i]]);

					FILE *fpout;
					fpout = fopen(filename, "wb");
					fprintf(fpout, "P6\n128 128\n255\n");
					fwrite(dataimage, sizeof(char), 128 * 128 * 3, fpout);
					fclose(fpout);
					*/

					/*
					//msフレーム前の画像を正解フォルダに割り当てる
					if ( (stepnumM[idno[0][i]] > ms) && (move[idno[0][i]][stepnumM[idno[0][i]] - ms - 1] != 0 ) ) { //stepnumM[id] が ms+1 以上
					char oldname[64];
					sprintf(oldname, "image_%.2f\\imagev\\image%d.%d.ppm", imr, idno[0][i], (stepnumM[idno[0][i]] - ms - 1) );

					char newname[64];
					sprintf(newname, "image_%.2f\\image%02d\\image%d.%d.ppm", imr, move[idno[0][i]][stepnumM[idno[0][i]] - ms - 1], idno[0][i], (stepnumM[idno[0][i]] - ms - 1) );
					//move[][]が0になっている

					if (rename(oldname, newname) == 0) {
					printf("%sから%sに名前を変更/移動しました", oldname, newname);
					}
					else {
					printf("名前の変更/移動に失敗しました");
					}
					}*/
				}
			}
		}
	}






	/*

	for (i = 0; i < 20; i++) {
	if (idno[0][i] == 0) {
	break;
	}
	else {
	glColor3ub(255, 0, 0);
	glBegin(GL_POINTS);
	for (j = 0; j < n; j++) { //nの数だけidno[0][i]の最新の軌跡の周りに円状に点を打つ
	glVertex2d(((point[idno[0][i]][stepnumM[idno[0][i]]][0] - 320) / 320.0) + r * cos(2.0 * 3.14 * (double)j / n), (-(point[idno[0][i]][stepnumM[idno[00][i]]][1] - 240) / 240.0) + r * sin(2.0 * 3.14 * (double)j / n));//点を打つ
	}
	glEnd();
	//idno[1][i]の軌跡を描く
	for (j = 1; j < stepnumM[idno[0][i]]; ++j) {
	if (point[idno[0][i]][j][0] == 0 && point[idno[0][i]][j][1] == 0) break;
	else {
	glBegin(GL_LINES);
	glVertex2d((point[idno[0][i]][j - 1][0] - 320) / 320.0, (-(point[idno[0][i]][j - 1][1] - 240) / 240.0));
	glVertex2d((point[idno[0][i]][j][0] - 320) / 320.0, (-(point[idno[0][i]][j][1] - 240) / 240.0)); //2次元の座標値を設定
	glEnd();
	}
	}
	//idno[1][i]以外の軌跡を描く
	glColor3ub(0, 0, 0);
	for (j = 0; j < 20; j++) {
	if (idno[0][j]==0  || idno[0][j]==idno[0][i]) {
	break;
	}
	else{

	//点
	glBegin(GL_POINTS);
	for (k = 0; k < n; k++) {
	glVertex2d(((point[idno[0][j]][stepnumM[idno[0][j]]][0] - 320) / 320.0) + r * cos(2.0 * 3.14 * (double)k/ n), (-(point[idno[0][j]][stepnumM[idno[0][j]]][1] - 240) / 240.0) + r * sin(2.0 * 3.14 * (double)k/ n));//点を打つ
	}
	glEnd();
	//軌跡
	for (k = 1; k < stepnumM[idno[0][j]]; ++k) {
	if (point[idno[0][j]][k][0] == 0 && point[idno[0][j]][k][1] == 0) break;
	else {
	//printf("pointx,idno[j](%d,%d)\n",point[idno[j]][k-1][0],idno[j]);
	glBegin(GL_LINES);
	glVertex2d((point[idno[0][j]][k - 1][0] - 320) / 320.0, (-(point[idno[0][j]][k - 1][1] - 240) / 240.0));
	glVertex2d((point[idno[0][j]][k][0] - 320) / 320.0, (-(point[idno[0][j]][k][1] - 240) / 240.0)); //2次元の座標値を設定
	glEnd();
	}
	}
	}
	}
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, 640, 480, GL_RGB, GL_UNSIGNED_BYTE, &outImage1[0][0][0]);

	for (j = 0; j < 480 / 2; j++) { //上下の反転
	for (k = 0; k < 640; k++) {
	for (l = 0; l < 3; l++) {
	tmp1 = outImage1[j][k][l];
	outImage1[j][k][l] = outImage1[479 - j][k][l];
	outImage1[479 - j][k][l] = tmp1;
	}
	}
	}

	if ((x - 180 * imr < 0) || (y - 180 * imr < 0)) {
	break;
	}
	else {
	//x-50*sqrt(2)~x+50*sqrt(2),(yも同様),の範囲を回転
	int x2, y2;
	//double imr = 1.5;//imr倍の範囲を切り取る
	for (l = 0; l < 180 * imr; l++) {
	for (j = 0; j < 180 * imr; j++) {
	for (k = 0; k < 3; k++) {
	x2 = rotsin(v[idno[0][i]][0], v[idno[0][i]][1])*(-180 * imr*0.5 + j) + rotcos(v[idno[0][i]][0], v[idno[0][i]][1])*(-180 * imr*0.9 + l) + x;//一般の点の回転(反時計回りに90-θ)
	y2 = -rotcos(v[idno[0][i]][0], v[idno[0][i]][1])*(-180 * imr*0.5 + j) + rotsin(v[idno[0][i]][0], v[idno[0][i]][1])*(-180 * imr*0.9 + l) + y;
	//x2 = 0.87*(-70 + j) + 0.5*(-70 + l) + x;//一般の点の回転(反時計回りに90-θ)
	//y2 = -0.5*(-70 + j) + 0.87*(-70 + l) + y;
	//x2 = cos*(- 70 + j) - sin*(- 70 + l) + x;//時計回りにθ回転
	//y2 = sin*(- 70 + j) + cos*(- 70 + l) + y;
	for (k = 0; k < 3; k++) {
	outImage2[y2][x2][k] = outImage1[y - (int)(162 * imr) + l][x - (int)(90 * imr) + j][k];
	}
	}
	}
	}
	for (l = 0; l < 128; l++) {//y
	for (j = 0; j < 128; j++) {//x
	for (k = 0; k < 3; k++) {
	dataimage[l][j][k] = outImage2[y - (int)(110 * imr) + (int)(imr*l)][x - (int)(64 * imr) + (int)(imr*j)][k];//dataimageとoutImage2の座標系の違いを修正
	}
	}
	}
	FILE *fpout;
	fpout = fopen("image1.0.ppm", "wb");
	fprintf(fpout, "P6\n128 128\n255\n");
	fwrite(dataimage, sizeof(char), 128 * 128 * 3, fpout);
	fclose(fpout);
	}
	}
	}*/
	
	/*
	//idごとの滞在フレーム数を出す
	if (frame == 67751) {
		double SS = 0;//sumstep
		printf("#id step \n");
		for (i = 1; i < 2549; i++) {
			printf("%d %d\n",i ,stepnumM[i]);
			SS = SS + stepnumM[i];
		}
		SS = SS / 2548;
		printf("SS %f", SS);
	}*/

	
	//idごとの速度を出す
	if (frame == 67751) {
		double VX = 0;
		double VY = 0;
		printf("#VX VY\n");
		for (i = 1; i < 2548; i++) {
			printf("%f %f\n", vm[i][0], vm[i][1]);
			VX = VX + vm[i][0];
			VY = VY + vm[i][1];
		}
		printf("平均 %f,%f", VX, VY);
	}
	

	/*
	for (i = 0; i < ms-1; i++) {
	for (j = 0; j < 20; j++) {
	idno[i + 1][j] = idno[i][j];//一個前のやつをひきつぐ
	}
	}*/


	for (i = 0; i < 20; i++) {
		idno[1][i] = idno[0][i];
	}


	//フレームごとに初期化
	for (i = 0; i < 20; i++) {
		idno[0][i] = 0;
	}



	/*
	//似てる軌跡を省く
	for (i = 1; i < Mid; ++i) {
	if (pointx[i] == 0) {//主要な軌跡を調査
	if (point[i][0][0] == 0 || point[i][0][1] == 0) {
	//pointx[i] = 1;
	break;
	}

	for (l = i + 1; l < id; ++l) {//(i+1~idまで判定)
	if (point[l][0][0] == 0 || point[l][0][1] == 0) break;
	if (pointx[l] == 0) {//主要な軌跡と比べる
	sum = 0;
	k = 0;//足した回数
	for (j = 1; j < MAXPOINTS; ++j) {
	if (point[l][j][0] == 0 || point[l][j][1] == 0 || point[i][j][0] == 0 || point[i][j][1] == 0) {
	break;
	}
	else {
	sum = sum + (point[i][j][0] - point[l][j][0])*(point[i][j][0] - point[l][j][0]) + (point[i][j][1] - point[l][j][1])*(point[i][j][1] - point[l][j][1]);
	k++;
	}
	}
	//if ( (k > 0) && (sum > 0) && ((sum / k )< 300) ) { //x,y
	if ((k > 0) && (sum > 0) && ((sum / k) < 80)) { //px,py
	pointx[l] = 1;//似ていたら主要でないとする
	break;
	}
	}
	}

	}
	else {
	break;
	}
	}*/

	/*
	glColor3ub(100, 100, 100);//今までの軌跡を描く
	for (k = 1; k < Mid; ++k) {
	if (point[k][0][0] == 0 || point[k][0][1] == 0) {
	break;
	}
	else if (pointx[k] == 1) {//主要でない軌跡は描かない
	break;
	}
	else {
	for (j = 1; j < MAXPOINTS; ++j) {
	if (point[k][j][0] == 0 || point[k][j][1] == 0) {
	//glVertex2d(0,0);
	//glVertex2d(0,0);
	break;
	}
	//else if ((point[k][j - 1][0] - point[k][j][0])*(point[k][j - 1][0] - point[k][j][0]) > 40000) {
	//printf("\nエラー！！ (id %d, x %d, y %d, xx %d, yy %d\n)", k, point[k][j][0], point[k][j][1], point[k][j - 1][0], point[k][j - 1][1]);
	//break;
	//}
	else {
	glBegin(GL_LINES);
	glVertex2d((point[k][j - 1][0] - 320) / 320.0, (-(point[k][j - 1][1] - 240) / 240.0));
	glVertex2d((point[k][j][0] - 320) / 320.0, (-(point[k][j][1] - 240) / 240.0)); //2次元の座標値を設定
	glEnd();
	}
	}
	}
	}
	*/

	//滞留点の色を滞留点がどれくらい周りにあるかによって変える

	/*
	//滞留点を描く
	glPointSize(4);
	glBegin(GL_POINTS);
	glColor3ub(255, 255, 0);
	for (i = 1; i < Mid; ++i) {
	for (j = 0; j < tai; ++j) {
	if (stay[i][j][0] == 0 || stay[i][j][1] == 0) break;
	else if (j > 100) break;//step数が多いものを省く
	else {
	//printf("\n 滞留！！ id %d(%d,%d) \n",i,stay[i][j][0],stay[i][j][1]);
	glVertex2d((stay[i][j][0] - 320) / 320.0, (-(stay[i][j][1] - 240) / 240.0));
	}
	}
	}
	glEnd();
	*/


	//時間を右下に文字で埋め込む
	glColor3d(0, 0, 0); //黒で
	sprintf(str, "%d/%02d/%02d %02d:%02d:%02d:%03d", year, month, day, hour, min, sec, msec);
	//時間：分：秒：ミリ秒
	glRasterPos2d(0.35, -0.95);
	for (i = 0; i<64; i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, str[i]);

	counter++;

	glFlush(); //強制描画			
	glutPostRedisplay(); //描画

}

void resize(int w, int h) {
	//簡易プログラムのためリサイズは考えない
}

void init(void) { //初期化
	glClearColor(1.0, 1.0, 1.0, 1.0); //窓を塗りつぶす色の設定
}

int main(int argc, char *argv[]) { // メイン関数


	int ix, iy, i;
	/*
	if ((gfp = fopen(ppmfile, "rb")) == NULL) {//画像が取り込めなければ
	printf("%s の背景画像を取り込めませんでした\n", ppmfile);
	printf("ファイルのパスをよく確認してください\n");
	system("pause");
	exit(0);
	}
	else { //画像を取り込めたら
	printf("%s の画像に軌跡を重ねます\n", ppmfile);
	fscanf(gfp, "P6\n640 480\n255\n");
	fread(gimage, sizeof(char), 640 * 480 * 3, gfp);
	fclose(gfp);
	//ガンマ変換で画像を薄くして見やすくする
	for (ix = 0; ix<640; ix++) {
	for (iy = 0; iy<480; iy++) {
	gimage[iy][ix][0] = 0xff * pow(gimage[iy][ix][0] / 255.0, 0.5);
	gimage[iy][ix][1] = 0xff * pow(gimage[iy][ix][1] / 255.0, 0.5);
	gimage[iy][ix][2] = 0xff * pow(gimage[iy][ix][2] / 255.0, 0.5);
	}
	}
	}*/

	if ((gfp = fopen(csvfile, "r")) == NULL) {//ファイルが取り込めなければ
		printf("%s の動線を取り込めませんでした\n", csvfile);
		printf("ファイルの名前をよく確認してください\n");
		system("pause");
		exit(0);
	}

	fscanf(gfp, "# file version %d", &ix); //ファイルのヘッダ部分を読み飛ばす
	if (ix != 3) {
		printf("csv ファイルのバージョンが違います\n");
		system("pause");
		exit(0);
	}
	printf("file version %d\n", ix);
	fscanf(gfp, ",,,,,,,,,,,,,,,,,,,,,,,,\n");
	fscanf(gfp, "# DATE-TIME,frame,no,ID,x,y,px,py,pz,ox,oy,oz,NS,wx,EW,wy,wz,subject,feature,gender,attribute,group,action,state,free\n");


	glutInitWindowPosition(100, 100); //窓を開く位置
	glutInitWindowSize(640, 480); //窓の大きさ 
	glutInit(&argc, argv); //GLUT および OpenGL 環境を初期化
	glutInitDisplayMode(GLUT_RGBA); //ディスプレイ表示モードの設定

	glutCreateWindow("SampleTrajectory"); //ウィンドウを開く
	glutDisplayFunc(display); //再描画

							  //glClearColor(0.5,0.5,0.0,0.0);//カラーバッファのクリアの色たぶん背景

	glutKeyboardFunc(keyboard); //キーボード
	glutMouseFunc(mouse); //マウス
	glutMotionFunc(motion); //ボタンを押している時のマウスの動き
	glutPassiveMotionFunc(motion2); //ボタンを押していない時のマウスの動き

	glutReshapeFunc(resize);
	glPixelZoom(1, -1); //OpenGL の座標系の上下を反対に

	init();
	glutMainLoop(); //無限ループ

	return 0;
}
