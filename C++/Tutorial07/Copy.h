#ifndef COPY_H
#define COPY_H
#include <string>
#include "Enemy.h"
using namespace std;
float MAX_COPY_LIFETIME = 2000.0f;
class Copy : public Enemy
{
public:
	Copy() {}
	Copy(string name, float deathTime, bool isDead, float speed, string enemyName) : Enemy(name, speed)
	{
		this->Name = name;
		this->EnemyName = enemyName;
		this->TimeToDie = deathTime;
		this->IsDead = isDead;
		this->Speed = 2.0f * speed;
	}

	string EnemyName;
	string Name;
	float TimeToDie;
	bool IsDead;
	float Speed;
private:
};
#endif