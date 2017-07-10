
/**
*/
class OptsTemplate {
  public:
    const uint8_t pin;

    OptsTemplate(const OptsTemplate* o) :
      pin(o->pin) {};

    OptsTemplate(uint8_t _pin) :
      pin(_pin) {};


  private:
};
