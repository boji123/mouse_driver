// driver.cpp : 定义控制台应用程序的入口点。
#include "stdafx.h"
#include "serial.h"
#include <math.h>
#include<windows.h>

void HandleSerial(byte * buffer);
void HandleGravity(byte * buffer,int mode);
void HandleAdjust(byte * buffer);
void HandleMove(byte * buffer);
void HandleFileOpen(byte * buffer);
//----------------------------------------
//这几个全局变量在加速度模块使用
double GravityBuffX,GravityBuffY;//用于去抖
double MiddleScreenX,MiddleScreenY;//主要用于找寻屏幕中点
double DiffX=0,DiffY=0;//用于校准
double x,y,z;//用于加速度模式，同时为校准保存上次的偏移
int mode;//加速度模式
//----------------------------------------
void _tmain(int argc, _TCHAR* argv[])
{
	MiddleScreenX=::GetSystemMetrics(SM_CXSCREEN)/2;//获取屏幕大小并计算中点
    MiddleScreenY=::GetSystemMetrics(SM_CYSCREEN)/2;

	CSerial SerialPort;
	int Port;
	cout<<"请输入端口号：";
	//Port=23;
	cin>>Port;
	cout<<"请输入加速度控制模式，1角度映射，2角度决定移动速度：";
	cin>>mode;
	int Baud=9600;

	if(SerialPort.IsOpened())
		SerialPort.Close();
	if(!SerialPort.Open(Port,Baud))
	{
		cout<<"无法打开计算机串口,程序将退出!!\r\n请检查串口连接或配置是否正常!!";
		while(1);
	}
	cout<<"串口已打开"<<endl;
	//while(1);
//----------------------------------------------------------------------------------------------
	byte buffer[100]={0};
	while(1)
	{	
		SerialPort.ReadData(buffer,99);//read into buffer
		if(buffer[0]!=0)
		{
			HandleSerial(buffer);
		}
		for(int i=0;i<100;i++)buffer[i]=0;//clear
		Sleep(1);//切忌自己写延迟函数，会导致CPU资源浪费
	}
}
//----------------------------------------------------------------------------------------------
void HandleSerial(byte * buffer)
{
	if(strncmp((char*)buffer,"gravity",7)==0)
	{
		//cout<<"gravity"<<endl;
		/*模式说明
			模式1：代谢法角度映射
			模式2：加速度积分
			模式3：角度决定速度
		//*/
		HandleGravity(buffer,mode);
	}
	else if(strncmp((char*)buffer,"move",4)==0)
	{
		//cout<<"move"<<endl;
		HandleMove(buffer);
	}
	else if(strncmp((char*)buffer,"adjust",6)==0)
	{
		cout<<"adjust"<<endl;
		HandleAdjust(buffer);
	}	
	else if(strncmp((char*)buffer,"open",4)==0)
	{
		cout<<"open"<<endl;
		HandleFileOpen(buffer);
	}
	else if(strncmp((char*)buffer,"leftdown",8)==0)
	{
		cout<<"leftdown"<<endl;
		mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
	}
	else if(strncmp((char*)buffer,"leftup",6)==0)
	{
		cout<<"leftup"<<endl;
		mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
	}
	else if(strncmp((char*)buffer,"rightdown",9)==0)
	{
		cout<<"rightdown"<<endl;
		mouse_event(MOUSEEVENTF_RIGHTDOWN,0,0,0,0);	
	}
	else if(strncmp((char*)buffer,"rightup",7)==0)
	{
		cout<<"rightup"<<endl;
		mouse_event(MOUSEEVENTF_RIGHTUP,0,0,0,0);		
	}
	else if(strncmp((char*)buffer,"leftclick",9)==0)
	{
		cout<<"leftclick"<<endl;
		mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
		mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
	}
	else if(strncmp((char*)buffer,"longclick",9)==0)
	{
		cout<<"longclick"<<endl;
		mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
	}
	else if(strncmp((char*)buffer,"longend",7)==0)
	{
		cout<<"longend"<<endl;
		mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
	}
}
void HandleGravity(byte * buffer,int mode)
{
	int i=0,begin,end;
	int n,m;
	char doubleStr[20]={0};


	//get x
	while(i++<99)
	{
		if(buffer[i]=='(')
			begin=i+1;
		if(buffer[i]==',')
		{
			end=i;
			break;
		}
	}
	for(m=begin,n=0;m<end;m++,n++)
		doubleStr[n]=buffer[m];
	doubleStr[n]=0;
	x=atof(doubleStr);


	//get y
	begin=end+1;
	while(i++<99)
	{
		if(buffer[i]==',')
		{
			end=i;
			break;
		}
	}
	for(m=begin,n=0;m<end;m++,n++)
		doubleStr[n]=buffer[m];
	doubleStr[n]=0;
	y=atof(doubleStr);


	//get z
	begin=end+1;
	while(i++<99)
	{
		if(buffer[i]==')')
		{
			end=i;
			break;
		}
	}
	for(m=begin,n=0;m<end;m++,n++)
		doubleStr[n]=buffer[m];
	doubleStr[n]=0;
	z=atof(doubleStr);

	x=x-DiffX;//误差校准
	y=y-DiffY;
	//cout<<z<<endl;	
	

	switch(mode)
	{
	case 1://代谢法角度映射
		//平滑去抖+校准偏差,这里的MiddleScreenY是灵敏度，原本应当为 灵敏度（=MiddleScreenX*倍率）*代谢系数0.03，这里为了简化运算把倍率和代谢系数合并。
		GravityBuffX = GravityBuffX*0.97+asin(-x/10)*MiddleScreenX*0.03*3;
		GravityBuffY = GravityBuffY*0.97+asin(-y/10)*MiddleScreenY*0.03*5;
		SetCursorPos(GravityBuffX+MiddleScreenX,GravityBuffY+MiddleScreenY);
		break;
	case 2://角度决定速度
		POINT curpos;
		GetCursorPos(&curpos);
		//cout<<"x:"<<x<<" y:"<<y<<endl;
		if(x>0)
			if(x<0.8)x=0;
			else x=x-0.8;
		if(x<0)
			if(x>-0.8)x=0;
			else x=x+0.8;

		if(y>0)
			if(y<0.7)y=0;
			else y=y-0.7;
		if(y<0)
			if(y>-0.7)y=0;
			else y=y+0.7;

		SetCursorPos(curpos.x+asin(-x/10)*MiddleScreenX/10,curpos.y+asin(-y/10)*MiddleScreenY/10);
		break;
	case 3://

		break;
	default:
		cout<<"mode error!";
	}
}
void HandleAdjust(byte * buffer)
{
	DiffX=DiffX+x;
	DiffY=DiffY+y;
	GravityBuffX=0;
	GravityBuffY=0;
	SetCursorPos(MiddleScreenX,MiddleScreenY);
}
void HandleMove(byte * buffer)
{
	int i=0,begin,end;
	int n,m;
	char doubleStr[20]={0};
	double x,y;

	//get x
	while(i++<99)
	{
		if(buffer[i]=='(')
			begin=i+1;
		if(buffer[i]==',')
		{
			end=i;
			break;
		}
	}
	for(m=begin,n=0;m<end;m++,n++)
		doubleStr[n]=buffer[m];
	doubleStr[n]=0;
	x=atof(doubleStr);


	//get y
	begin=end+1;
	while(i++<99)
	{
		if(buffer[i]==')')
		{
			end=i;
			break;
		}
	}
	for(m=begin,n=0;m<end;m++,n++)
		doubleStr[n]=buffer[m];
	doubleStr[n]=0;
	y=atof(doubleStr);


	//cout<<x<<' '<<y<<' '<<z<<endl;
	POINT curpos;
	GetCursorPos(&curpos);
	SetCursorPos(curpos.x+x*2,curpos.y+y*2);
}
void HandleFileOpen(byte * buffer)
{
	int i=0,begin,end;
	int n,m;
	char fileName[20]={0};


	//get x
	while(i++<99)
	{
		if(buffer[i]=='(')
			begin=i+1;
		if(buffer[i]==')')
		{
			end=i;
			break;
		}
	}
	for(m=begin,n=0;m<end;m++,n++)
		fileName[n]=buffer[m];
	fileName[n]=0;
	if(n!=0)
	{
		cout<<fileName<<endl;
		if(ShellExecute(0,"open",fileName,"","",SW_SHOWNORMAL)<HINSTANCE(32))//可以打开当前路径下的文件，但是不能处理中文
			cout<<"file name error or not exist!"<<endl;
	}
	else
	{
		cout<<"empty file name!"<<endl;
	}
}