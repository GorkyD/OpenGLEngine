#pragma once

class Rect
{
	public:
		Rect(){}
		Rect(int width,int height) : width(width), height(height) {}
		Rect(int left, int top, int width, int height) : width(width), height(height), left(left), top(top) {}
		Rect(const Rect& rect) : width(rect.width), height(rect.height), left(rect.left), top(rect.top) {}

		int width = 0, height = 0, left = 0, top = 0;
};
