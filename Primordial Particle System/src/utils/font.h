#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>

class Font
{
    sf::Font m_font_;
    sf::Text m_text_;
    sf::RenderWindow* m_window_;

public:
    // Constructor taking font size and file location
    Font(sf::RenderWindow* window, const unsigned font_size, const std::string& font_location) : m_window_(window)
    {
        if (!m_font_.loadFromFile(font_location))
        {
            std::cerr << "[ERROR]: Failed to load font from: " << font_location << '\n';
            return;
        }

        m_text_.setFont(m_font_);
        m_text_.setFillColor(sf::Color::White);
        set_font_size(font_size);
    }

    // Function to set font size
    void set_font_size(const unsigned font_size)
    {
        m_text_.setCharacterSize(font_size);
    }

    // Function to draw text on the window
    void draw(const sf::Vector2f& position, const std::string& string_text, const bool centered = false)
    {
        m_text_.setString(string_text);

        // Calculate text bounds
        const sf::FloatRect text_bounds = m_text_.getLocalBounds();
        sf::Vector2f text_position = position;

        // Adjust position for centering if needed
        if (centered)
        {
            text_position.x -= text_bounds.width / 2.0f;
            text_position.y -= text_bounds.height / 2.0f;
        }

        m_text_.setPosition(text_position);
        m_window_->draw(m_text_);
    }
};