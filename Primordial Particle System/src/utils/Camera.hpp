#pragma once

#include <SFML/Graphics.hpp>

/*
A class for managing translation and zoom of a SFML surface 
 */

class Camera
{
public:
	float m_currentScroll = 1.f;


private:
	sf::RenderWindow* m_window_ptr_ = nullptr;
	sf::View m_view_{};

	sf::Vector2f m_mouse_position_before_{};
	sf::Vector2f m_delta_{};

	float m_zoom_strength_ = 0.04f;

	// for view toggle.
	sf::FloatRect original_view{};
	sf::FloatRect new_view{};


public:
	explicit Camera(sf::RenderWindow* window_ptr, const float scale = 1.f)
		: m_window_ptr_(window_ptr)
	{
		const auto dims = sf::Vector2f(m_window_ptr_->getSize());
		original_view = { 0, 0, dims.x, dims.y };
		new_view = original_view;
		m_view_ = sf::View(original_view);

		m_currentScroll *= scale;
		zoom(0);
	}


	void update()
	{
		const auto mouse_pos = sf::Vector2f(sf::Mouse::getPosition(*m_window_ptr_));
		m_delta_ = mouse_pos - m_mouse_position_before_;
		m_mouse_position_before_ = mouse_pos;
		
	}


	void translate()
	{
		// Invert the signs of the components to move in the correct direction
		const sf::Vector2f final_translation = { -m_delta_.x / m_currentScroll, -m_delta_.y / m_currentScroll };
		m_view_.move(final_translation);
		new_view = m_view_.getViewport();

		update_window_view();
	}


	void zoom(const float delta_scroll)
	{
		// Get the mouse position before the zoom
		const sf::Vector2f before_mouse_pos = m_window_ptr_->mapPixelToCoords(sf::Mouse::getPosition(*m_window_ptr_), m_view_);

		const float scale = get_zoom_scale(delta_scroll);

		// Calculate the new zoom level
		m_currentScroll *= scale;

		// Set the zoom level of the view
		m_view_.setSize(m_window_ptr_->getSize().x / m_currentScroll, m_window_ptr_->getSize().y / m_currentScroll);

		// Get the mouse position after the zoom
		const sf::Vector2f after_mouse_pos = m_window_ptr_->mapPixelToCoords(sf::Mouse::getPosition(*m_window_ptr_), m_view_);

		// Adjust the view to keep the mouse position fixed
		const sf::Vector2f offset = before_mouse_pos - after_mouse_pos;
		m_view_.move(offset);

		update_window_view();
	}


	sf::Vector2f get_world_mouse_pos() const
	{
		const sf::Vector2i mouse_position = sf::Mouse::getPosition(*m_window_ptr_);
		const sf::Vector2f view_mouse_position = m_window_ptr_->mapPixelToCoords(mouse_position, m_view_);

		return view_mouse_position;
	}


	
private:
	void update_window_view() const
	{
		m_window_ptr_->setView(m_view_);
	}

	[[nodiscard]] float get_zoom_scale(const float delta) const
	{
		if (delta > 0) 
			return 1.0f + m_zoom_strength_;
		return
		1.0f - m_zoom_strength_;
	}
};