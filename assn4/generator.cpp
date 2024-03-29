//
// BetterCarryingEachOther 2016.03.17
// automatic enemy generation
//

#include <iostream>
#include <cstdlib>
#include <cmath>

#include "generator.h"
#include "enemy.h"
#include "view.h"
#include "utility.h"
#include "deco.h"
#include "game.h"
#include "river.h"
#include "null.h"
#include "resource.h"
#include "light.h"

Generator::Generator(enum type t, float y, bool left, float spd, float gapMin, float gapMax)
	: Object::Object() {
	this->t = t;
	
	locate(vec3(0.0, y, 0.0));
	this->left = left;
	this->spd = spd;
	this->gapMin = gapMin;
	this->gapMax = gapMax;

	for (float dx = frandRange(0.0, gapMax) - Game::getWidthLimit(); dx < Game::getWidthLimit(); ) {
		create()->move(vec3(left ? dx : -dx, 0.0, 0.0));
		alarmSet();
		dx += alarm * spd;
	}

	name = "Generator";
}

void Generator::alarmSet() {
	alarm = frandRange(gapMin, gapMax) / spd;
}

void Generator::update() {
	alarm -= 1.0;

	if (alarm <= 0.0) {
		create();
		alarmSet();
	}

	Object::update();
}

Object* Generator::create() {
	Object* o;

	switch (t) {
	case TYPE_NONE:
		break;
	case TYPE_ENEMY:
		o = new Car();
		break;
	case TYPE_LOG:
		o = new Log();
		break;
	}

	o->locate(vec3(left ? -Game::getWidthLimit() : Game::getWidthLimit(), pos.y, 0.0));
	o->setXvel(left ? spd : -spd);
	push(o);

	return o;
}

float MetaGenerator::difRate = 0.00005;

MetaGenerator::MetaGenerator() : Object::Object() {
	difficulty = 0.0;
	target = Game::getGrid() * 4.0;
	name = "Metagenerator";
}

void MetaGenerator::placeRoads() {
	NullLimiter* nl = new NullLimiter(target);
	push(nl);

	int lane;
	if (difficulty < 0.25)
		lane = rand() % 2 + 1;
	else if (difficulty < 0.5)
		lane = rand() % 3 + 1;
	else if (difficulty < 0.75)
		lane = rand() % 2 + 2;

	float flane = (float)lane;
	float spdMin = 1.0 + difficulty * 5.0;
	float spdMax = 2.0 + difficulty * 8.0;
	float gapMin = 320.0 - difficulty * 200.0;
	float gapMax = 640.0 - difficulty * 200.0;

	Road* r = new Road(lane);
	r->locate(vec3(0.0, target, 0.0));
	nl->push(r);

	const float wlimit = Game::getWidthLimit() - Game::getGrid() * 1.5;

	for (float i = 0.0; i < 2.0 * flane; i += 1.0) {
		float y = target + i * Game::getGrid();
		nl->push(
			new Generator(
				Generator::TYPE_ENEMY, y, i >= flane,
				frandRange(spdMin, spdMax), gapMin, gapMax)
			);
		nl->push(
			new Deco(
				vec3(wlimit, y, 0.0), 
				vec3(0.0, 0.0, 90.0), 
				&Resource::tunnel, 
				&Resource::Tex::tunnel,
				&Resource::Norm::tunnel
				)
			);
		nl->push(
			new Deco(
				vec3(-wlimit, y, 0.0), 
				vec3(0.0, 0.0, -90.0), 
				&Resource::tunnel, 
				&Resource::Tex::tunnel,
				&Resource::Norm::tunnel
				)
			);
	}
	target += Game::getGrid() * (flane * 2.0);
}

void MetaGenerator::placeRivers() {
	NullLimiter* nl = new NullLimiter(target);
	push(nl);

	const float wlimit = Game::getWidthLimit() - Game::getGrid() * 1.5;

	for (int repeat = rand() % 3 + 1; repeat > 0; repeat--) {
		bool left = rand() % 2 ? true : false;

		float spdMin = 0.5 + difficulty * 3.0;
		float spdMax = 0.5 + difficulty * 6.0;
		float gapMin = 200.0 + difficulty * 100.0;
		float gapMax = 320.0 + difficulty * 300.0;

		nl->push(new Water(target));
		nl->push(
			new Generator(
				Generator::TYPE_LOG, target, left,
				frandRange(spdMin, spdMax), gapMin, gapMax)
			);
		nl->push(
			new Deco(
				vec3(-wlimit, target, 0.0), 
				vec3(0.0, 0.0, 180.0), 
				&Resource::drain, 
				&Resource::Tex::tunnel,
				&Resource::Norm::tunnel
				)
			);
		nl->push(
			new Deco(
				vec3(wlimit, target, 0.0), 
				vec3(0.0, 0.0, 0.0), 
				&Resource::drain, 
				&Resource::Tex::tunnel,
				&Resource::Norm::tunnel
				)
			);

		target += Game::getGrid();
	}
}

void MetaGenerator::placeSafezone() {
	NullLimiter* nl = new NullLimiter(target);
	push(nl);

	int repeat = rand() % 3 + 1;

	nl->push(new Deco(
		vec3(0.0, target, 0.0),
		&Resource::grass[repeat - 1],
		&Resource::Tex::grass,
		&Resource::Norm::grass
		));

	for (int i = 0; i < repeat; i++) {
		placeTrees(target + Game::getGrid() * i);
	}

	for (int i = 0; i < repeat + 1; i++) {
		if (frand() < 0.5) {
			int limit = (int)(Game::getWidthLimit() / Game::getGrid());
			int ind = rand() % (2 * limit - 1) - limit;
			nl->push(
				new StreetLight(
					vec3(
						((float)ind + 0.5) * Game::getGrid(),
						target + Game::getGrid() * ((float)i - 0.5),
						0.0
					))
				);
		}
	}

	target += Game::getGrid() * repeat;
}

void MetaGenerator::update() {
	difficulty = fminf(View::getY() * difRate, 1.0);

	if (View::getY() + View::getZfar() > target) {
		if (frand() > 0.2)
			placeRoads();
		else
			placeRivers();
		placeSafezone();
	}

	Object::update();
}

void MetaGenerator::placeTrees(float y) {
	NullLimiter* nl = new NullLimiter(y);
	push(nl);

	nl-> push(new Tree(Player::getXlimit() + Game::getGrid(), y));
	nl->push(new Tree(-Player::getXlimit() - Game::getGrid(), y));

	if (frand() > 0.6 + 0.4 * difficulty)
		return;

	int num = rand() % 2 + 1;
	int plimit = (int)(Player::getXlimit() / Game::getGrid());
	int limit = (int)(Game::getWidthLimit() / Game::getGrid());
	int location = rand() % (plimit * 2 + 2 - num) - plimit;

	for (int i = 0; i < num; i++) {
		nl->push(new Tree((float)(location + i) * Game::getGrid(), y));
	}
}