package Classes.Player;

// CLASE PRUEBA

public class player {
    public int x, y, vx, vy;
    public int lives = 3;
    public boolean isDead = false;
    public int score = 0;
    
    public player(int x,int y){ 
        this.x=x; 
        this.y=y;

    }

    public void decreaseLife(){
        this.lives--;
        if(this.lives <= 0){
            this.isDead = true;
        }
    }

    public void increaseScore(int points){
        this.score += points;
    }

    public void increaseLife(){
        this.lives++;
    }

    public int getLives(){
        return this.lives;
    }

    public void setLives(int lives){
        this.lives = lives;
    }

    public void setScore(int score){
        this.score = score;
    }




}
