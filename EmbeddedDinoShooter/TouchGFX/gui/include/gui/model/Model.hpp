#ifndef MODEL_HPP
#define MODEL_HPP

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    int getHighScore() const { return highScore; }
    void saveHighScore(int score)
    {
        if (score > highScore)
        {
            highScore = score;
        }
    }

    int getSelectedStage() const { return selectedStage; }
    void setSelectedStage(int stage) { selectedStage = stage; }

protected:
    ModelListener* modelListener;
    int highScore;
    int selectedStage;
};

#endif // MODEL_HPP
