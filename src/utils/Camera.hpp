#pragma once
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/System/Vector2.hpp>


class Camera
{
public:
    sf::View m_view_{};

    float m_pan_friction_ = 8.0f;   // lower = slides longer after release
    float m_pan_lerp_ = 10.0f;  // lower = more lag when starting/changing direction

private:
    sf::RenderWindow* m_window_ = nullptr;

    // zoom
    float m_current_zoom_ = 1.f;
    float m_target_zoom_ = 1.f;
    float m_zoom_strength_ = 0.08f;
    float m_smooth_zoom_speed_ = 8.0f;

    // pan
    sf::Vector2f m_mouse_prev_{};
    sf::Vector2f m_pan_velocity_{};

    bool m_is_panning_ = false;

public:
    explicit Camera(sf::RenderWindow* window_ptr, const float scale = 1.f)
        : m_window_(window_ptr), m_current_zoom_(scale), m_target_zoom_(scale)
    {
        const auto dims = sf::Vector2f(m_window_->getSize());
        m_view_ = sf::View(sf::Rect<float>{{0.f, 0.f}, { dims.x, dims.y }});
        m_view_.setSize(dims / m_current_zoom_);
        update_window_view();
    }

    // call at the start of each frame, before event handling
    void update(const float dt)
    {
        const sf::Vector2f mouse_pos = sf::Vector2f(sf::Mouse::getPosition(*m_window_));
        const sf::Vector2f raw_delta = mouse_pos - m_mouse_prev_;
        m_mouse_prev_ = mouse_pos;

        if (m_is_panning_)
        {
            // smoothly accelerate toward the mouse movement rather than snapping to it
            m_pan_velocity_ += (raw_delta - m_pan_velocity_) * m_pan_lerp_ * dt;
        }
        else
        {
            // decelerate with friction when released
            m_pan_velocity_ -= m_pan_velocity_ * m_pan_friction_ * dt;
        }

        m_view_.move(-m_pan_velocity_ / m_current_zoom_);

        smooth_zoom(dt);
        update_window_view();
    }

    void begin_pan() { m_is_panning_ = true; }
    void end_pan() { m_is_panning_ = false; }

    void zoom(const float scroll_delta)
    {
        if (scroll_delta > 0)
            m_target_zoom_ *= (1.0f + m_zoom_strength_);
        else
            m_target_zoom_ *= (1.0f - m_zoom_strength_);
    }

    sf::Vector2f get_world_mouse_pos() const
    {
        return m_window_->mapPixelToCoords(sf::Mouse::getPosition(*m_window_), m_view_);
    }

    template<typename T>
    sf::Vector2f window_pos_to_world_pos(const sf::Vector2<T> window_pos) const
    {
        return m_window_->mapPixelToCoords(static_cast<sf::Vector2i>(window_pos), m_view_);
    }

    template<typename F>
    void call_draw_at_absolute_position(F&& fn)
    {
        const auto saved = m_window_->getView();
        m_window_->setView(m_window_->getDefaultView());
        fn();
        m_window_->setView(saved);
    }

    void update_window_view() const
    {
        m_window_->setView(m_view_);
    }

private:
    void smooth_zoom(const float dt)
    {
        if (std::abs(m_current_zoom_ - m_target_zoom_) < 0.0001f)
            return;

        const sf::Vector2f before = get_world_mouse_pos();

        m_current_zoom_ += (m_target_zoom_ - m_current_zoom_) * m_smooth_zoom_speed_ * dt;
        m_view_.setSize(sf::Vector2f(m_window_->getSize()) / m_current_zoom_);

        const sf::Vector2f after = get_world_mouse_pos();
        m_view_.move(before - after);
    }
};