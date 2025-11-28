package Classes.Player;

/**
 * Lightweight data model for a game player.
 * <p>
 * This class stores position, velocity, lives, death state, and score.
 * It exposes minimal mutators for lives and score management and keeps all
 * fields package-visible for performance in tight server loops.
 */
public class player {
    /** X position in pixels (world coordinates). */
    public int x;
    /** Y position in pixels (world coordinates). */
    public int y;
    /** X velocity in pixels per tick. */
    public int vx;
    /** Y velocity in pixels per tick. */
    public int vy;

    /** Remaining lives; reaching {@code 0} sets {@link #isDead} to true. */
    public int lives = 3;
    /** True when the player has no lives left. */
    public boolean isDead = false;
    /** Accumulated score points. */
    public int score = 0;

    /**
     * Creates a player at the given position.
     *
     * @param x initial X position in pixels
     * @param y initial Y position in pixels
     */
    public player(int x, int y){
        this.x = x;
        this.y = y;
    }

    /**
     * Decreases lives by one and marks the player as dead when lives reach zero.
     */
    public void decreaseLives(){
        this.lives--;
        if (this.lives <= 0){
            this.isDead = true;
        }
    }

    /**
     * Increases the score by the specified number of points.
     *
     * @param points points to add (may be negative if needed by game rules)
     */
    public void increaseScore(int points){
        this.score += points;
    }

    /**
     * Increases lives by one (revival logic is handled elsewhere).
     */
    public void increaseLives(){
        this.lives++;
    }

    /**
     * @return current number of lives.
     */
    public int getLives(){
        return this.lives;
    }

    /**
     * @return current score.
     */
    public int getScore(){
        return this.score;
    }

    /**
     * Sets the number of lives; does not toggle {@link #isDead}.
     *
     * @param lives new lives value
     */
    public void setLives(int lives){
        this.lives = lives;
    }

    /**
     * Sets the score to an absolute value.
     *
     * @param score new score value
     */
    public void setScore(int score){
        this.score = score;
    }
}
