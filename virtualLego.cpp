////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: ��â�� Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <cmath>

#define BallNum 60 //병록 추가 변수

IDirect3DDevice9* Device = NULL;

// window size
const int Width  = 512;
const int Height = 1024;

//공 위치 배열: 위치 배열 { 좌표 배열 {} } 
const float spherePos[60][2] = {
	//top
	{2,0.21} , {2,0.63} , {2,1.05} , {2, 1.47},
	{2,-0.21} , {2,-0.63} , {2,-1.05} , {2, -1.47},
	//bottom
	{-2.94,0.21} , {-2.94,0.63} , {-2.94,1.05} , {-2.94, 1.47},
	{-2.94,-0.21} , {-2.94,-0.63} , {-2.94,-1.05} , {-2.94, -1.47},

	//left
	{1.7, 1.77},
	{1.4, 2.07}, {0.98, 2.07}, {0.56, 2.07}, {0.14, 2.07},{-0.28, 2.07},
	{-0.7, 2.07}, {-1.12, 2.07}, {-1.54, 2.07},{-1.96, 2.07},{-2.34, 2.07},
	{-2.64, 1.77},

	//right
	{1.7, -1.77},
	{1.4, -2.07}, {0.98, -2.07}, {0.56, -2.07}, {0.14, -2.07},{-0.28, -2.07},
	{-0.7, -2.07}, {-1.12, -2.07}, {-1.54, -2.07},{-1.96, -2.07},{-2.34, -2.07},
	{-2.64, -1.77},

	//cross line-vertical
	{1.7, 0}, {1.4, 0}, {0.98, 0}, {0.56, 0}, {0.14, 0},{-0.28, 0},
	{-0.7, 0}, {-1.12, 0}, {-1.54, 0},{-1.96, 0},{-2.34, 0},{-2.64, 0},
	//cross line-horizontal
	{-0.28,0.42} , {-0.28,0.84} , {-0.28, 1.26} , {-0.28, 1.68},
	{-0.28,-0.42} , {-0.28,-0.84} , {-0.28,-1.26} , {-0.28, -1.68}
};
// initialize the color of each ball (ball0 ~ ball3)
const D3DXCOLOR sphereColor[3] = { d3d::RED, d3d::YELLOW, d3d::WHITE };

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
//공이나 벽 위치 설정할 때 함수 parameter로 넘겨줄 행렬
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private:
	float   center_x, center_y, center_z; // 3차원 좌표
	float   m_radius; // 구 반경
	float	m_velocity_x; // 구 x속도
	float	m_velocity_z; // 구 z속도

public:
	CSphere(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_radius = 0;
		m_velocity_x = 0;
		m_velocity_z = 0;
		m_pSphereMesh = NULL;
	}
	~CSphere(void) {}

public:
	bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
			return false;
		return true;
	}

	void destroy(void)
	{
		if (m_pSphereMesh != NULL) {
			m_pSphereMesh->Release();
			m_pSphereMesh = NULL;
		}
	}

	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pSphereMesh->DrawSubset(0);
	}

	/* bool hasIntersected(CSphere& ball)
	 {
		 D3DXVECTOR3 current_ball_pos = ball.getCenter();
		 double distance = sqrt(pow(current_ball_pos.x - center_x, 2) + pow(current_ball_pos.z - center_z, 2));
		 if (distance < 0.42)
		 {
			 return true;
		 }
		 return false;
	 }*/

	void hitBy(CSphere& ball)
	{
		D3DXVECTOR3 current_ball_pos = ball.getCenter();
		double deltaX = current_ball_pos.x - center_x;
		double deltaZ = current_ball_pos.z - center_z;
		double distance = sqrt(deltaX * deltaX + deltaZ * deltaZ);
		if (distance < 0.5)
		{
			double hitAngle = acosf(deltaX / distance); // arccos
			this->setPower(m_velocity_x * sin(hitAngle), m_velocity_z * cos(hitAngle));
		}
	}

	void ballUpdate(float timeDiff)
	{
		const float TIME_SCALE = 3.3;
		D3DXVECTOR3 cord = this->getCenter();
		double vx = abs(this->getVelocity_X());
		double vz = abs(this->getVelocity_Z());

		if (vx > 0.01 || vz > 0.01)
		{
			float tX = cord.x + TIME_SCALE * timeDiff * m_velocity_x;
			float tZ = cord.z + TIME_SCALE * timeDiff * m_velocity_z;

			//correction of position of ball
			// 공이 벽에 부딪힐 때 공의 위치 수정이 필요하기 때문에 이 부분의 주석을 제거해주세요.
			// 제거함
			if(tX >= (4.5 - M_RADIUS))
				tX = 4.5 - M_RADIUS;
			else if(tX <=(-4.5 + M_RADIUS))
				tX = -4.5 + M_RADIUS;
			else if(tZ <= (-3 + M_RADIUS))
				tZ = -3 + M_RADIUS;
			else if(tZ >= (3 - M_RADIUS))
				tZ = 3 - M_RADIUS;*/
			
			this->setCenter(tX, cord.y, tZ);
		}
		else { this->setPower(0, 0); }
		//this->setPower(this->getVelocity_X() * DECREASE_RATE, this->getVelocity_Z() * DECREASE_RATE);
		/*double rate = 1 -  (1 - DECREASE_RATE)*timeDiff * 400;
		if(rate < 0 )
			rate = 0;*/
		this->setPower(getVelocity_X(), getVelocity_Z());
	}

	double getVelocity_X() { return this->m_velocity_x; }
	double getVelocity_Z() { return this->m_velocity_z; }

	void setPower(double vx, double vz)
	{
		this->m_velocity_x = vx;
		this->m_velocity_z = vz;
	}

	void setCenter(float x, float y, float z)
	{
		D3DXMATRIX m;
		center_x = x;	center_y = y;	center_z = z;
		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	float getRadius(void)  const { return (float)(M_RADIUS); }
	const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
	D3DXVECTOR3 getCenter(void) const
	{
		D3DXVECTOR3 org(center_x, center_y, center_z);
		return org;
	}

private:
	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh* m_pSphereMesh;

};



// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:

	float					m_x;
	float					m_z;
	float                   m_width;
	float                   m_depth;
	float					m_height;

public:
	CWall(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_width = 0;
		m_depth = 0;
		m_pBoundMesh = NULL;
	}
	~CWall(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		m_width = iwidth;
		m_depth = idepth;

		if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
			return false;
		return true;
	}
	void destroy(void)
	{
		if (m_pBoundMesh != NULL) {
			m_pBoundMesh->Release();
			m_pBoundMesh = NULL;
		}
	}
	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pBoundMesh->DrawSubset(0);
	}

	//bool hasIntersected(CSphere& ball) 
	//{
	//	// Insert your code here.
	//	D3DXVECTOR3 current_ball_pos = ball.getCenter();
	//	if ((current_ball_pos.z + 0.21) >= (m_width / 2) || (current_ball_pos.z - 0.21) <= -1*(m_width/2))
	//	{
	//		return true;
	//	}

	//	if ((current_ball_pos.x + 0.21) >= (m_height / 2))
	//	{
	//		return true;
	//	}

	//	return false;
	//}

	// Display에 따로 구현
	/*void hitBy(CSphere& ball)
	{
	}    */

	void setPosition(float x, float y, float z)
	{
		D3DXMATRIX m;
		this->m_x = x;
		this->m_z = z;

		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	float getHeight(void) const { return M_HEIGHT; }



private:
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh* m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
	CLight(void)
	{
		static DWORD i = 0;
		m_index = i++;
		D3DXMatrixIdentity(&m_mLocal);
		::ZeroMemory(&m_lit, sizeof(m_lit));
		m_pMesh = NULL;
		m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		m_bound._radius = 0.0f;
	}
	~CLight(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
	{
		if (NULL == pDevice)
			return false;
		if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
			return false;

		m_bound._center = lit.Position;
		m_bound._radius = radius;

		m_lit.Type = lit.Type;
		m_lit.Diffuse = lit.Diffuse;
		m_lit.Specular = lit.Specular;
		m_lit.Ambient = lit.Ambient;
		m_lit.Position = lit.Position;
		m_lit.Direction = lit.Direction;
		m_lit.Range = lit.Range;
		m_lit.Falloff = lit.Falloff;
		m_lit.Attenuation0 = lit.Attenuation0;
		m_lit.Attenuation1 = lit.Attenuation1;
		m_lit.Attenuation2 = lit.Attenuation2;
		m_lit.Theta = lit.Theta;
		m_lit.Phi = lit.Phi;
		return true;
	}
	void destroy(void)
	{
		if (m_pMesh != NULL) {
			m_pMesh->Release();
			m_pMesh = NULL;
		}
	}
	bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return false;

		D3DXVECTOR3 pos(m_bound._center);
		D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
		D3DXVec3TransformCoord(&pos, &pos, &mWorld);
		m_lit.Position = pos;

		pDevice->SetLight(m_index, &m_lit);
		pDevice->LightEnable(m_index, TRUE);
		return true;
	}

	void draw(IDirect3DDevice9* pDevice)
	{
		if (NULL == pDevice)
			return;
		D3DXMATRIX m;
		D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
		pDevice->SetTransform(D3DTS_WORLD, &m);
		pDevice->SetMaterial(&d3d::WHITE_MTRL);
		m_pMesh->DrawSubset(0);
	}

	D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
	DWORD               m_index;
	D3DXMATRIX          m_mLocal;
	D3DLIGHT9           m_lit;
	ID3DXMesh* m_pMesh;
	d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables - 레벨 설계 부분
// -----------------------------------------------------------------------------
CWall	g_legoPlane;
CWall	g_legowall[4];
CSphere	g_targetBall[60];
CSphere	g_target_blueball;
CSphere g_plate_whiteball;
CLight	g_light;
LPD3DXFONT Font;
RECT rt;


double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
}

// initialization - 
bool Setup()
{

	SetRect(&rt, 125, 50 , 0, 0); // text크기
	int i;

	D3DXMatrixIdentity(&g_mWorld);
	D3DXMatrixIdentity(&g_mView);
	D3DXMatrixIdentity(&g_mProj);

	// 공이 위치할 평면
	if (false == g_legoPlane.create(Device, -1, -1, 9, 0.03f, 6, d3d::GREEN)) return false;
	g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

	// 벽 개수와 벽 위치 - setPosition 함수
	if (false == g_legowall[0].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
	g_legowall[0].setPosition(4.56f, 0.12f, 0.0f);
	if (false == g_legowall[1].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
	g_legowall[1].setPosition(-4.56f, 0.12f, 0.0f);
	if (false == g_legowall[2].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[2].setPosition(0.0f, 0.12f, 3.06f);
	if (false == g_legowall[3].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[3].setPosition(0.0f, 0.12f, -3.06f);


	// 공 개수와 위치 - setPosition 함수
	for (i = 0; i < 60; i++) {
		if (false == g_targetBall[i].create(Device, sphereColor[1])) return false;
		g_targetBall[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
		g_targetBall[i].setPower(0, 0);
	}

	// create blue ball for set direction
	if (false == g_target_blueball.create(Device, d3d::BLUE)) return false;
	g_target_blueball.setCenter(3.5, (float)M_RADIUS, 0);

	if (false == g_plate_whiteball.create(Device, d3d::WHITE)) return false;
	g_plate_whiteball.setCenter(4.12, (float)M_RADIUS, 0);

	// light setting - 안 건들여도 됌
	D3DLIGHT9 lit;
	::ZeroMemory(&lit, sizeof(lit));
	lit.Type = D3DLIGHT_POINT;
	lit.Diffuse = d3d::WHITE;
	lit.Specular = d3d::WHITE * 0.9f;
	lit.Ambient = d3d::WHITE * 0.9f;
	lit.Position = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
	lit.Range = 100.0f;
	lit.Attenuation0 = 0.0f;
	lit.Attenuation1 = 0.9f;
	lit.Attenuation2 = 0.0f;
	if (false == g_light.create(Device, lit))
		return false;

	// Position and aim the camera. - 평면 상단으로 고정시켜야됌 수정
	D3DXVECTOR3 pos(5.0f, 14.0f, 0.0f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);

	// Set the projection matrix.
	D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
		(float)Width / (float)Height, 1.0f, 100.0f);
	Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

	// Set render states.
	Device->SetRenderState(D3DRS_LIGHTING, TRUE);
	Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
	Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

	g_light.setLight(Device, g_mWorld);
	return true;
}

void Cleanup(void)
{
	g_legoPlane.destroy();
	for (int i = 0; i < 4; i++) {
		g_legowall[i].destroy();
	}
	destroyAllLegoBlock();
	g_light.destroy();
}




// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
// 런타임
bool Display(float timeDelta)
{

	if (Device)
	{
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
		Device->BeginScene();

		if (g_target_blueball.getCenter().x < 4.26)
		{
			g_target_blueball.ballUpdate(timeDelta);
			//g_plate_whiteball.ballUpdate(timeDelta);
		}
		else
		{
			D3DXCreateFont(Device, 50, 0, FW_NORMAL, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &Font);

			Font->DrawText(NULL, "Game over", -1, &rt, DT_NOCLIP, D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f));
		}

			
	
		D3DXVECTOR3 current_ball_pos = g_target_blueball.getCenter();
		double current_velocity_z = g_target_blueball.getVelocity_Z();
		double current_velocity_x = g_target_blueball.getVelocity_X();

		if ((current_ball_pos.z + 0.21) >= 2.8 || (current_ball_pos.z - 0.21) <= -2.8)
		{
			g_target_blueball.setPower(current_velocity_x, current_velocity_z * -1);
		}

		if ((current_ball_pos.x - 0.21) <= -4.1)
		{
			g_target_blueball.setPower(current_velocity_x * -1, current_velocity_z);
		}

		for (int i = 0; i < 60; i++)
		{
			D3DXVECTOR3 targetball_pos = g_targetBall[i].getCenter();
			double deltaX = targetball_pos.x - current_ball_pos.x;
			double deltaZ = targetball_pos.z - current_ball_pos.z;
			double distance = sqrt(deltaX * deltaX + deltaZ * deltaZ);
			//if (distance < 0.42)
			//{
			//	double hitAngle = acosf(deltaX / distance); // arccos
			//	double total_velocity = sqrt(g_target_blueball.getVelocity_X() * g_target_blueball.getVelocity_X() + g_target_blueball.getVelocity_Z() * g_target_blueball.getVelocity_Z());
			//	g_target_blueball.setPower(total_velocity * sin(hitAngle), total_velocity * cos(hitAngle));
			//	g_targetBall[i].setCenter(100.0f, 0.0f, 100.0f);
			//}
			if (distance <= 0.42) {
				double hitAngle = acosf(deltaX / distance);
				double total_velocity = sqrt(g_target_blueball.getVelocity_X() * g_target_blueball.getVelocity_X() + g_target_blueball.getVelocity_Z() * g_target_blueball.getVelocity_Z());
				if (targetball_pos.z <= current_ball_pos.z) {
					if (hitAngle <= 0.785398) {
						g_target_blueball.setPower(-current_velocity_x, current_velocity_z);
						g_targetBall[i].setCenter(100.0f, 0.0f, 100.0f);
					}
					else if (hitAngle > 0.785398) {
						g_target_blueball.setPower(current_velocity_x, -current_velocity_z);
						g_targetBall[i].setCenter(100.0f, 0.0f, 100.0f);
					}
				}
				else if (targetball_pos.z >= current_ball_pos.z) {
					if (hitAngle <= 0.785398) {
						g_target_blueball.setPower(current_velocity_x, -current_velocity_z);
						g_targetBall[i].setCenter(100.0f, 0.0f, 100.0f);
					}
					else if (hitAngle > 0.785398) {
						g_target_blueball.setPower(-current_velocity_x, current_velocity_z);
						g_targetBall[i].setCenter(100.0f, 0.0f, 100.0f);
					}
				}
			}
		}

		D3DXVECTOR3 whiteball_pos = g_plate_whiteball.getCenter();
		double deltaX = whiteball_pos.x - current_ball_pos.x;
		double deltaZ = whiteball_pos.z - current_ball_pos.z;
		double distance = sqrt(deltaX * deltaX + deltaZ * deltaZ);
		//if (distance <= 0.41)
		//{
		//	double hitAngle = acosf(deltaX / distance); // arccos
		//	double total_velocity = sqrt(g_target_blueball.getVelocity_X() * g_target_blueball.getVelocity_X() + g_target_blueball.getVelocity_Z() * g_target_blueball.getVelocity_Z());
		//	g_target_blueball.setPower(total_velocity * sin(hitAngle), total_velocity * cos(hitAngle));
		//}

		if (distance <= 0.42) {
			double hitAngle = acosf(deltaX / distance);
			double total_velocity = sqrt(g_target_blueball.getVelocity_X() * g_target_blueball.getVelocity_X() + g_target_blueball.getVelocity_Z() * g_target_blueball.getVelocity_Z());
			if (whiteball_pos.z <= current_ball_pos.z) {
				if (hitAngle <= 0.785398) {
					g_target_blueball.setPower(-current_velocity_x, current_velocity_z);
				}
				else if (hitAngle > 0.785398) {
					g_target_blueball.setPower(current_velocity_x, -current_velocity_z);
				}
			}
			else if (whiteball_pos.z >= current_ball_pos.z) {
				if (hitAngle <= 0.785398) {
					g_target_blueball.setPower(current_velocity_x, -current_velocity_z);
				}
				else if (hitAngle > 0.785398) {
					g_target_blueball.setPower(-current_velocity_x, current_velocity_z);
				}
			}
		}



		// draw plane, walls, and spheres
		g_legoPlane.draw(Device, g_mWorld);

		for (int i = 0; i < 4; i++) {
			g_legowall[i].draw(Device, g_mWorld);
		}

		for (int k = 0; k < 60; k++)
		{
			g_targetBall[k].draw(Device, g_mWorld);
		}

		g_target_blueball.draw(Device, g_mWorld);
		g_plate_whiteball.draw(Device, g_mWorld);

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
		Device->SetTexture(0, NULL);
	}
	return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool wire = false;
	static bool isReset = true;
	static int old_x = 0;
	static int old_y = 0;
	static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;
	static const double VELOCITY = -0.8; //속도 설정
	static bool isGameStarted = false; //게임 시작 판정

	D3DXVECTOR3 current_center_white = g_plate_whiteball.getCenter(); // 현재 받침 공 위치 받아오기
	D3DXVECTOR3 current_center_blue = g_target_blueball.getCenter(); // 현재 움직여야할 공 위치 받아오기

	switch (msg) {
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		break;
	}
	case WM_KEYDOWN:
	{
		switch (wParam) {
		case VK_ESCAPE:
			::DestroyWindow(hwnd);
			break;
		case VK_RETURN:
			if (NULL != Device) {
				wire = !wire;
				Device->SetRenderState(D3DRS_FILLMODE,
					(wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
			}
			break;

		case VK_SPACE:
			isGameStarted = true;
			g_target_blueball.setPower(VELOCITY, VELOCITY); // 속도 업데이트
			break;

		case VK_LEFT:
			if (current_center_white.z >= -2.8)
			{
				if (isGameStarted)
				{
					g_plate_whiteball.setCenter(current_center_white.x, current_center_white.y, current_center_white.z - 0.4);
					//g_plate_whiteball.setPower(0.0f, -1.0f);
				}
				else
				{
					//게임이 시작하지 않았으면 plate를 따라서 z값을 같이 움직임
					g_plate_whiteball.setCenter(current_center_white.x, current_center_white.y, current_center_white.z - 0.4);
					g_target_blueball.setCenter(current_center_blue.x, current_center_blue.y, current_center_white.z - 0.4);
		
				}
			}
			break;

		case VK_RIGHT:
			if (current_center_white.z <= 2.8)
			{
				if (isGameStarted)
				{
					g_plate_whiteball.setCenter(current_center_white.x, current_center_white.y, current_center_white.z + 0.4);
					//g_plate_whiteball.setPower(0.0f, 1.0f);
				}
				else
				{
					g_plate_whiteball.setCenter(current_center_white.x, current_center_white.y, current_center_white.z + 0.4);
					g_target_blueball.setCenter(current_center_blue.x, current_center_blue.y, current_center_white.z + 0.4);
	
				}
			}
			break;

		}
		break;
	}
	}

	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE prevInstance,
	PSTR cmdLine,
	int showCmd)
{
	srand(static_cast<unsigned int>(time(NULL)));

	if (!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	if (!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop(Display);

	Cleanup();

	Device->Release();

	return 0;
}