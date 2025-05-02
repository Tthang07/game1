#include "Player.h"

void Player::moveLeft() { if (x > 0) x -= speed; }
void Player::moveRight() { if (x < SCREEN_WIDTH - PLAYER_WIDTH) x += speed; }
void Player::moveUp() { if (y > 0) y -= speed; }
void Player::moveDown() { if (y < SCREEN_HEIGHT - PLAYER_HEIGHT) y += speed; }
