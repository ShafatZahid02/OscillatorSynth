// OscillatorGUI.java
import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

public class OscillatorGUI {
    static {
        System.loadLibrary("Oscillator"); // Load the C++ library
    }

    // Native methods
    public native void addOscillator(int type, double frequency, double amplitude, double attack, double decay, double sustain, double release);
    public native void setDelayParameters(double delayTime, double feedback, double mix);
    public native void setReverbParameters(double decay, double mix);
    public native void initializeAudioBackend();
    public native void cleanupAudioBackend();

    public static void main(String[] args) {
        OscillatorGUI gui = new OscillatorGUI();
        gui.initializeAudioBackend(); // Initialize audio backend

        JFrame frame = new JFrame("Oscillator Controller");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(600, 400);

        JPanel panel = new JPanel();
        panel.setLayout(new GridLayout(10, 2));

        // Oscillator controls
        JTextField frequencyField = new JTextField("440.0");
        JTextField amplitudeField = new JTextField("1.0");
        JTextField attackField = new JTextField("0.1");
        JTextField decayField = new JTextField("0.1");
        JTextField sustainField = new JTextField("0.7");
        JTextField releaseField = new JTextField("0.2");

        JComboBox<String> typeComboBox = new JComboBox<>(new String[]{"Sine", "Square", "Sawtooth"});

        JButton addButton = new JButton("Add Oscillator");
        addButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int type = typeComboBox.getSelectedIndex();
                double frequency = Double.parseDouble(frequencyField.getText());
                double amplitude = Double.parseDouble(amplitudeField.getText());
                double attack = Double.parseDouble(attackField.getText());
                double decay = Double.parseDouble(decayField.getText());
                double sustain = Double.parseDouble(sustainField.getText