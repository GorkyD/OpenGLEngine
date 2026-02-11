#pragma once

class ORect
{
	public:
		ORect(){}
		ORect(int width,int height) : width(width), height(height) {}
		ORect(int left, int top, int width, int height) : width(width), height(height), left(left), top(top) {}
		ORect(const ORect& rect) : width(rect.width), height(rect.height), left(rect.left), top(rect.top) {}

		int width = 0, height = 0, left = 0, top = 0;
};																	