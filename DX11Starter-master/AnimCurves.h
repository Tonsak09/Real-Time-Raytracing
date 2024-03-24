#pragma once
#include <iostream>

// Which curve type
#define EASE_IN_SINE 0
#define EASE_OUT_SINE 1
#define EASE_IN_OUT_SINE 2
#define EASE_IN_QUAD 3
#define EASE_OUT_QUAD 4
#define EASE_IN_OUT_QUAD 5
#define EASE_IN_CUBIC 6
#define EASE_OUT_CUBIC 7
#define EASE_IN_OUT_CUBIC 8
#define EASE_IN_QUART 9
#define EASE_OUT_QUART 10
#define EASE_IN_OUT_QUART 11
#define EASE_IN_QUINT 12
#define EASE_OUT_QUINT 13
#define EASE_IN_OUT_QUINT 14
#define EASE_IN_EXPO 15
#define EASE_OUT_EXPO 16
#define EASE_IN_OUT_EXPO 17
#define EASE_IN_CIRC 18
#define EASE_OUT_CIRC 19
#define EASE_IN_OUT_CIRC 20
#define EASE_IN_BACK 21
#define EASE_OUT_BACK 22
#define EASE_IN_OUT_BACK 23
#define EASE_IN_ELASTIC 24
#define EASE_OUT_ELASTIC 25
#define EASE_IN_OUT_ELASTIC 26
#define EASE_IN_BOUNCE 27
#define EASE_OUT_BOUNCE 28
#define EASE_IN_OUT_BOUNCE 29

const float PI = 3.14159265358979323846f;



static float EaseInSine(float x)
{
	return 1 - std::cos((x * PI) / 2.0f);
}

static float EaseOutSine(float x)
{
	return std::sin((x * PI) / 2.0f);
}

static float EaseInOutSine(float x)
{
	return -(std::cos(PI * x) - 1.0f) / 2.0f;
}

static float EaseInQuad(float x)
{
	return x * x;
}

static float EaseOutQuad(float x)
{
	return 1.0f - (1.0f - x) * (1.0f - x);
}

static float EaseInOutQuad(float x)
{
	return x < 0.5f ? 2.0f * x * x : 1.0f - std::pow(-2.0f * x + 2.0f, 2.0f) / 2.0f;
}

static float EaseInCubic(float x)
{
	return x * x * x;
}

static float EaseOutCubic(float x)
{
	return 1.0f - std::pow(1.0f - x, 3.0f);
}

static float EaseInOutCubic(float x)
{
	return x < 0.5f ? 4.0f * x * x * x : 1.0f - std::pow(-2.0f * x + 2.0f, 3.0f) / 2.0f;
}

static float EaseInQuart(float x)
{
	return x * x * x * x;
}

static float EaseOutQuart(float x)
{
	return 1.0f - std::pow(1.0f - x, 4.0f);
}

static float EaseInOutQuart(float x)
{
	return  x < 0.5f ? 8.0f * x * x * x * x : 1.0f - std::pow(-2.0f * x + 2.0f, 4.0f) / 2.0f;
}

static float EaseInQuint(float x)
{
	return x * x * x * x * x;
}

static float EaseOutQuint(float x)
{
	return 1.0f - std::pow(1.0f - x, 5.0f);
}

static float EaseInOutQuint(float x)
{
	return x < 0.5f ? 16.0f * x * x * x * x * x : 1.0f - std::pow(-2.0f * x + 2.0f, 5.0f) / 2.0f;
}

static float EaseInExpo(float x)
{
	return (x == 0) ? 0 : std::pow(2.0f, 10.0f * x - 10.0f);
}

static float EaseOutExpo(float x)
{
	return (x == 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * x);
}

static float EaseInOutExpo(float x)
{
	return (x == 0)
		? 0
		: (x == 1)
		? 1.0f
		: x < 0.5f ? std::pow(2.0f, 20.0f * x - 10.0f) / 2.0f
		: (2.0f - std::pow(2.0f, -20.0f * x + 10.0f)) / 2.0f;
}

static float EaseInCirc(float x)
{
	return 1.0f - std::sqrt(1.0f - std::pow(x, 2.0f));
}

static float EaseOutCirc(float x)
{
	return std::sqrt(1.0f - std::pow(x - 1.0f, 2.0f));
}

static float EaseInOutCirc(float x)
{
	return (x < 0.5f)
		? (1.0f - std::sqrt(1.0f - std::pow(2.0f * x, 2.0f))) / 2.0f
		: (std::sqrt(1.0f - std::pow(-2.0f * x + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}

static float EaseInBack(float x, float c1 = 1.70158f)
{
	float c3 = c1 + 1.0f;
	return c3 * x * x * x - c1 * x * x;
}

static float EaseOutBack(float x, float c1 = 1.70158f)
{
	float c3 = c1 + 1.0f;
	return 1.0f + c3 * std::pow(x - 1.0f, 3.0f) + c1 * std::pow(x - 1.0f, 2.0f);
}

static float EaseInOutBack(float x, float c1 = 1.70158f)
{
	float c2 = c1 * 1.525f;

	return x < 0.5f
		? (std::pow(2.0f * x, 2.0f) * ((c2 + 1.0f) * 2.0f * x - c2)) / 2.0f
		: (std::pow(2.0f * x - 2.0f, 2.0f) * ((c2 + 1.0f) * (x * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
}

static float EaseInElastic(float x, float c4 = (2.0f * PI) / 3.0f)
{
	return (x == 0)
		? 0
		: (x == 1)
		? 1.0f
		: -std::pow(2.0f, 10.0f * x - 10.0f) * std::sin((x * 10.0f - 10.75f) * c4);
}

static float EaseOutElastic(float x, float c4 = (2.0f * PI) / 3.0f)
{
	return (x == 0)
		? 0
		: (x == 1)
		? 1
		: std::pow(2.0f, -10.0f * x) * std::sin((x * 10.0f - 0.75f) * c4) + 1;
}

static float EaseInOutElastic(float x, float c5 = (2.0f * PI) / 4.5f)
{
	return (x == 0)
		? 0
		: (x == 1)
		? 1
		: x < 0.5f
		? -(std::pow(2.0f, 20.0f * x - 10.0f) * std::sin((20.0f * x - 11.125f) * c5)) / 2.0f
		: (std::pow(2.0f, -20.0f * x + 10.0f) * std::sin((20.0f * x - 11.125f) * c5)) / 2.0f + 1.0f;
}

static float EaseOutBounce(float x, float n1 = 7.5625f, float d1 = 2.75f)
{
	if (x < 1.0f / d1)
	{
		return n1 * x * x;
	}
	else if (x < 2.0f / d1)
	{
		return n1 * (x -= 1.5f / d1) * x + 0.75f;
	}
	else if (x < 2.5f / d1)
	{
		return n1 * (x -= 2.25f / d1) * x + 0.9375f;
	}
	else
	{
		return n1 * (x -= 2.625f / d1) * x + 0.984375f;
	}
}

static float EaseInBounce(float x)
{
	return 1.0f - EaseOutBounce(1.0f - x);
}

static float EaseInOutBounce(float x)
{
	return x < 0.5f
		? (1.0f - EaseOutBounce(1.0f - 2.0f * x)) / 2.0f
		: (1.0f + EaseOutBounce(2.0f * x - 1.0f)) / 2.0f;
}


static float GetCurveByIndex(int curveType, float p)
{
	switch (curveType)
	{
	case EASE_IN_SINE:
		return EaseInSine(p);
	case EASE_OUT_SINE:
		return EaseOutSine(p);
	case EASE_IN_OUT_SINE:
		return EaseInOutSine(p);
	case EASE_IN_QUAD:
		return EaseInQuad(p);
	case EASE_OUT_QUAD:
		return EaseOutQuad(p);
	case EASE_IN_OUT_QUAD:
		return EaseInOutQuad(p);
	case EASE_IN_CUBIC:
		return EaseInCubic(p);
	case EASE_OUT_CUBIC:
		return EaseOutCubic(p);
	case EASE_IN_OUT_CUBIC:
		return EaseInOutCubic(p);
	case EASE_IN_QUART:
		return EaseInQuart(p);
	case EASE_OUT_QUART:
		return EaseOutQuart(p);
	case EASE_IN_OUT_QUART:
		return EaseInOutQuart(p);
	case EASE_IN_QUINT:
		return EaseInQuint(p);
	case EASE_OUT_QUINT:
		return EaseOutQuint(p);
	case EASE_IN_OUT_QUINT:
		return EaseInOutQuint(p);
	case EASE_IN_EXPO:
		return EaseInExpo(p);
	case EASE_OUT_EXPO:
		return EaseOutExpo(p);
	case EASE_IN_OUT_EXPO:
		return EaseInOutExpo(p);
	case EASE_IN_CIRC:
		return EaseInCirc(p);
	case EASE_OUT_CIRC:
		return EaseOutCirc(p);
	case EASE_IN_OUT_CIRC:
		return EaseInOutCirc(p);
	case EASE_IN_BACK:
		return EaseInBack(p);
	case EASE_OUT_BACK:
		return EaseOutBack(p);
	case EASE_IN_OUT_BACK:
		return EaseInOutBack(p);
	case EASE_IN_ELASTIC:
		return EaseInElastic(p);
	case EASE_OUT_ELASTIC:
		return EaseOutElastic(p);
	case EASE_IN_OUT_ELASTIC:
		return EaseInOutElastic(p);
	case EASE_IN_BOUNCE:
		return EaseInBounce(p);
	case EASE_OUT_BOUNCE:
		return EaseOutBounce(p);
	case EASE_IN_OUT_BOUNCE:
		return EaseInOutBounce(p);
	default:
		return 1.0f;
		break;
	}
}