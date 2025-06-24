#pragma once

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

template<typename T>
class SquareImage
{
public:
	SquareImage() = default;
	~SquareImage() = default;

	[[nodiscard]] const godot::Vector<T>& get_data() const { return data; }
	[[nodiscard]] int32_t get_size() const { return size; }

	void resize(size_t new_size)
	{
		size = new_size;
		data.resize_zeroed(new_size);
		buffer.resize_zeroed(new_size);
	}

	template<typename F>
	void read_pixels(const godot::Vector2& center, const real_t radius, F functor) const
	{
		const auto min = godot::Vector2i(
			Math::clamp(static_cast<int32_t>(Math::round(center.x - radius)), 0, size),
			Math::clamp(static_cast<int32_t>(Math::round(center.y - radius)), 0, size)
		);
		const auto max = godot::Vector2i(
			Math::clamp(static_cast<int32_t>(Math::round(center.x + radius)), 0, size),
			Math::clamp(static_cast<int32_t>(Math::round(center.y + radius)), 0, size)
		);
		for (auto x = min.x; x <= max.x; ++x)
		{
			for (auto y = min.y; y <= min.y; ++y)
			{
				const auto d = center.distance_to(godot::Vector2(x, y));
				const auto t = Math::max((real_t)1.0 - (d / radius), (real_t)0.0);
				functor(get_index(x, y), x, y, t);
			}
		}
	}

	template<typename F>
	void write_pixels(const godot::Vector2& center, const real_t radius, F functor)
	{
		const auto min = godot::Vector2i(
			Math::clamp(static_cast<int32_t>(Math::round(center.x - radius)), 0, size),
			Math::clamp(static_cast<int32_t>(Math::round(center.y - radius)), 0, size)
		);
		const auto max = godot::Vector2i(
			Math::clamp(static_cast<int32_t>(Math::round(center.x + radius)), 0, size),
			Math::clamp(static_cast<int32_t>(Math::round(center.y + radius)), 0, size)
		);

		// Operate on buffer
		for (auto x = min.x; x <= max.x; ++x)
		{
			for (auto y = min.y; y <= max.y; ++y)
			{
				const auto i = get_index(x, y);
				const auto d = center.distance_to(godot::Vector2(x, y));
				const auto t = Math::max((real_t)1.0 - (d / radius), (real_t)0.0);
				buffer.set(i, functor(data[i], x, y, t));
			}
		}

		// Apply changes to data
		for (auto x = min.x; x <= max.x; ++x)
		{
			for (auto y = min.y; y <= max.y; ++y)
			{
				const auto index = get_index(x, y);
				data.set(index, buffer[index]);
			}
		}
	}

	void add(const real_t amount, const godot::Vector2& center, const real_t radius, const real_t exp)
	{
		if (radius <= 0.0 || amount <= 0.0)
			return;
		write_pixels(center, radius, [this, amount, exp](const int32_t index, const int32_t x, const int32_t y, const real_t t) -> T
		{
			return move_T_towards(data[index], data[index] + amount, t, exp);
		});
	}

	void smooth(const real_t strength, const godot::Vector2& center, const real_t radius, const real_t exp)
	{
		if (radius <= 0.0 || strength <= 0.0)
			return;
		write_pixels(center, radius, [this, exp](const int32_t index, const int32_t x, const int32_t y, const real_t t) -> T
		{
			const auto ia = get_index(x - 1, y);
			const auto ib = get_index(x + 1, y);
			const auto ic = get_index(x, y - 1);
			const auto id = get_index(x, y + 1);
			const auto average = (data[ia] + data[ib] + data[ic] + data[id] + data[index]) * (real_t)0.2;
			return move_T_towards(data[index], average, t, exp);
		});
	}

	void flatten(const real_t strength, const godot::Vector2& center, const real_t radius, const real_t exp)
	{
		if (radius <= 0.0 || strength <= 0.0)
			return;
		real_t average = 0.0;
		int32_t num_pixels = 0;
		read_pixels(center, radius, [this, &average, &num_pixels](const int32_t index, const int32_t x, const int32_t y, const real_t t) -> void
		{
			average += data[index];
			num_pixels++;
		});

		if (num_pixels <= 0)
			return;
		
		average /= num_pixels;
		write_pixels(center, radius, [this, average, exp](const int32_t index, const int32_t x, const int32_t y, const real_t t) -> T
		{
			return move_T_towards(data[index], average, t, exp);
		});
	}

	T bilinear_sample(const godot::Vector2& p)
	{		
		const auto pi = Vector2i(
			static_cast<int>(p.x),
			static_cast<int>(p.y)
		);

		const auto v1 = data[get_index(pi.x + 0, pi.y + 0)];
		const auto v2 = data[get_index(pi.x + 1, pi.y + 0)];
		const auto v3 = data[get_index(pi.x + 0, pi.y + 1)];
		const auto v4 = data[get_index(pi.x + 1, pi.y + 1)];

		const auto tx = (p.x - Math::floor(p.x));
		const auto ty = (p.y - Math::floor(p.y));

		const auto a = Math::lerp(v1, v2, tx);
		const auto b = Math::lerp(v3, v4, tx);
		return Math::lerp(a, b, ty);
	}

	T get_pixel(const int32_t x, const int32_t y) const
	{
		return data[get_index(x, y)];
	}

	T get_pixel(const godot::Vector2i& p) const
	{
		return get_pixel(p.x, p.y);
	}

	void set_pixel(const int32_t x, const int32_t y, const T& value)
	{
		data.set(get_index(x, y), value);
	}

	void set_pixel(const godot::Vector2i& p, const T& value)
	{
		set_pixel(p.x, p.y, value);
	}

	void fill(const T& value)
	{
		data.fill(value);
	}

private:

	int32_t get_index(const int32_t x, const int32_t y) const
	{
		return Math::clamp(x, 0, size) + Math::clamp(y, 0, size) * size;
	}

	static T move_T_towards(const T from, const T to, const real_t p, const real_t exp)
	{
		return from + godot::UtilityFunctions::ease(p, exp) * (to - from);
	}

	godot::Vector<T> data;
	godot::Vector<T> buffer;
	int32_t size;
};