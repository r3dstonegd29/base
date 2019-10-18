#include "curve.h"
#include <fstream>

curve_base::curve_base(std::string path)
{
	std::string real_path = "./data/curves/" + path + ".txt";
	std::ifstream file;
	file.open(real_path);
	if (file.is_open())
	{
		std::string stream;
		file.seekg(0, std::ios::end);
		stream.reserve(file.tellg());
		file.seekg(0, std::ios::beg);
		stream.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

		while (1)
		{
			size_t s;
			s = stream.find("KF");
			if (s > stream.size()) break;
			stream = stream.substr(s);

			s = stream.find("time");
			float time = (float)std::atof(stream.substr(s + 7, stream.find(";") - s - 7).c_str());

			vec3 value;
			s = stream.find("vals");
			stream = stream.substr(s+7);
			value.x = (float)std::atof(stream.substr(0, stream.find(",")).c_str());
			stream = stream.substr(stream.find(",") + 1);
			value.y = (float)std::atof(stream.substr(0, stream.find(",")).c_str());
			stream = stream.substr(stream.find(",") + 1);
			value.z = (float)std::atof(stream.substr(0, stream.find(",")).c_str());

			m_frames.push_back({ value, time });
		}
		file.close();
		m_name = path;
	}
}

float curve_base::duration() const
{
	return m_frames.back().second;
}

vec3 curve_line::evaluate(float t)const
{
	if (t <= 0.0f || m_frames.size() == 1)
		return m_frames.front().first;
	if (t >= duration())
		return m_frames.back().first;

	for (size_t i = 0; i < m_frames.size() - 1; i++)
		if (t < m_frames[i + 1].second)
			return map(t, m_frames[i].second, m_frames[i + 1].second,
				m_frames[i].first, m_frames[i + 1].first);
	return{};
}

vec3 curve_hermite::evaluate(float t)const
{
	if (t <= 0.0f || m_frames.size() == 1)
		return m_frames.front().first;
	if (t >= duration())
		return m_frames.back().first;

	for (size_t i = 0; i < m_frames.size() - 3; i += 3)
	{
		if (t < m_frames[i + 3].second)
		{
			float c = coef(m_frames[i].second, m_frames[i + 3].second, t);

			vec3 P0 = m_frames[i].first;
			vec3 P1 = m_frames[i + 3].first;

			vec3 T0 = m_frames[i + 1].first;
			vec3 T1 = m_frames[i + 2].first;

			return (2.0f*(P0 - P1) + T0 + T1)*(c*c*c) + (3.0f*(P1 - P0) - 2.0f*T0 - T1)*(c*c) + T0 * c + P0;
		}
	}
	return{};
}

vec3 curve_catmull::evaluate(float t)const
{
	if (t <= 0.0f || m_frames.size() == 1)
		return m_frames.front().first;
	if (t >= duration())
		return m_frames.back().first;

	/*LINEAR*/
	for (size_t i = 0; i < m_frames.size() - 1; i++)
		if (t < m_frames[i + 1].second)
			return map(t, m_frames[i].second, m_frames[i + 1].second,
				m_frames[i].first, m_frames[i + 1].first);
	/*LINEAR*/

	for (size_t i = 0; i < m_frames.size() - 1; i++)
		if (t < m_frames[i + 1].second)
		{
			if (i == 0)
			{
				return map(t, m_frames[i].second, m_frames[i + 1].second,
					m_frames[i].first, m_frames[i + 1].first);
			}
			else if (i == m_frames.size() - 2)
			{
				return map(t, m_frames[i].second, m_frames[i + 1].second,
					m_frames[i].first, m_frames[i + 1].first);
			}
			else
			{
				float c = coef(m_frames[i].second, m_frames[i + 3].second, t);

				vec3 P0 = m_frames[i - 1].first;
				vec3 P1 = m_frames[i].first;
				vec3 P2 = m_frames[i + 1].first;
				vec3 P3 = m_frames[i + 2].first;

				return (-P0 + 3.0f*P1 - 3.0f*P2 + P3)*(c*c*c) + (2.0f*P0 - 5.0f*P1 + 4.0f*P2 - P3)*(c*c) + (P2 - P0)*0.5f*c + P1;
			}
		}
	return{};
}

vec3 curve_bezier::evaluate(float t)const
{
	if (t <= 0.0f || m_frames.size() == 1)
		return m_frames.front().first;
	if (t >= duration())
		return m_frames.back().first;

	for (size_t i = 0; i < m_frames.size() - 3; i += 3)
	{
		if (t < m_frames[i + 3].second)
		{
			float c = coef(m_frames[i].second, m_frames[i + 3].second, t);

			vec3 P0 = m_frames[i].first;
			vec3 C0 = m_frames[i + 1].first;
			vec3 C1 = m_frames[i + 2].first;
			vec3 P1 = m_frames[i + 3].first;

			vec3 m = lerp(P0, C0, c);
			vec3 n = lerp(C0, C1, c);
			vec3 o = lerp(C1, P1, c);

			vec3 r = lerp(m, n, c);
			vec3 s = lerp(n, o, c);

			return lerp(r, s, c);
		}
	}
	return{};
}