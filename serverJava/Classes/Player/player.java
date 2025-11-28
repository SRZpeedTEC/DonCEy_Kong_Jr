package Classes.Player;

// CLASE PRUEBA
//class that represents a player in the game
public class player {
    public int x, y, vx, vy;
    public int lives = 3;
    public boolean isDead = false;
    public int score = 0;


    //params x and y represent the position of the player
    //returns a new player object
    public player(int x,int y){ 
        this.x=x; 
        this.y=y;

    }

    //params vx and vy represent the velocity of the player
    //returns nothing
    public void decreaseLives(){
        this.lives--;
        if(this.lives <= 0){
            this.isDead = true;
        }
    }


    //params points represent the points to be added to the score
    //returns nothing
    public void increaseScore(int points){
        this.score += points;
    }


    //params none
    //returns nothing
    public void increaseLives(){ 
        this.lives++;
    }


    //params none
    //returns the number of lives of the player
    public int getLives(){
        return this.lives;
    }

    //params none
    //returns the score of the player
    public int getScore(){
        return this.score;
    }


    //params lives represent the number of lives to be set
    //returns nothing
    public void setLives(int lives){
        this.lives = lives;
    }


    //params score represent the score to be set
    //returns nothing
    public void setScore(int score){
        this.score = score;
    }




}
