#include "game.h"

Game& Game::getInstance() {
	static Game game;
	return game;
}

void Game::updateGame() {
	// make a copy of Game data to avoid locking resource for too long
	Data data_copy;
	{
		std::lock_guard<std::mutex> lock(data_mutex);
		data_copy = data;
	}

	auto now = std::chrono::high_resolution_clock::now();
	const float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - data_copy.last_updated).count();

	// update spaceships
	for (Spaceship& s : data_copy.spaceships) {
		s.pos += s.vector;
	}

	// update bullets
	for (Bullet& b : data_copy.bullets) {
		b.pos += b.vector;
	}

	// update asteroids
	for (Asteroid& a : data_copy.asteroids) {
		a.pos += a.vector;
	}

	// check for collisions
	for (const Bullet& b : data_copy.bullets) {
		//for (const)
	}

	// update last updated time
	data_copy.last_updated = std::chrono::high_resolution_clock::now();

	{
		std::lock_guard<std::mutex> lock(data_mutex);
		data = data_copy;
	}
}


std::vector<char> Game::Data::toBytes() {
	std::vector<char> buf;

	// num spaceships
	buf.push_back((char)spaceships.size());
	std::vector<char> bytes;

	for (const Spaceship& s : spaceships) {
		// only stuff integral to rendering/interaction is sent
		// vector is not sent

		buf.push_back(s.sid & 0xff);		// spaceship sid

		bytes = Server::t_to_bytes(s.pos.x);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos x (4 bytes)

		bytes = Server::t_to_bytes(s.pos.y);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos y (4 bytes)

		bytes = Server::t_to_bytes(s.rotation);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// rotation in degrees (4 bytes)

		buf.push_back(s.lives_left);						// lives left (1 byte)
	}

	// num bullets
	buf.push_back((char)bullets.size());

	for (const Bullet& b : bullets) {
		// vector is not sent, not integral to rendering

		buf.push_back(b.sid & 0xff);		// bullet sid

		bytes = Server::t_to_bytes(b.pos.x);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos x (4 bytes)

		bytes = Server::t_to_bytes(b.pos.y);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos y (4 bytes)
	}

	// num asteroids
	buf.push_back((char)asteroids.size());

	for (const Asteroid& a : asteroids) {
		// vector not sent, not needed for rendering

		bytes = Server::t_to_bytes(a.pos.x);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos x (4 bytes)

		bytes = Server::t_to_bytes(a.pos.y);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos y (4 bytes)
	}

	return buf;
}

void Game::Data::reset() {
	bullets.clear();
	asteroids.clear();
	last_updated = std::chrono::high_resolution_clock::now();
}
