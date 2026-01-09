use std::collections::VecDeque;

const MAX_DELAY_SECONDS: usize = 3;
const TWO_PI: f32 = 6.283185307179586;

pub struct Delay {
    input_buffer: Box<VecDeque<f32>>,
    output_buffer: Box<VecDeque<f32>>,
    allpass_buffer: Box<VecDeque<f32>>,
    sample_rate: usize,
    delay_samples: usize,
    target_delay_samples: usize,
    feedback_amount: f32,
    character_amount: f32,
    smoothing_samples: usize,
    current_smooth_step: usize,
    allpass_delay: usize,
    diffusion_amount: f32,
    lfo_phase: f32,
    lfo_frequency: f32,
    wow_flutter_depth: f32,
    noise_seed: u32,
}

impl Delay {
    fn new(sample_rate: usize, delay_samples: usize, feedback_amount: f32) -> Self {
        let buffer_capacity = MAX_DELAY_SECONDS * sample_rate;
        let mut input_buffer = Box::new(VecDeque::with_capacity(buffer_capacity));
        let mut output_buffer = Box::new(VecDeque::with_capacity(buffer_capacity));

        let allpass_size = (sample_rate as f32 * 0.005) as usize;
        let mut allpass_buffer = Box::new(VecDeque::with_capacity(allpass_size));

        input_buffer.resize(buffer_capacity, 0.0);
        output_buffer.resize(buffer_capacity, 0.0);
        allpass_buffer.resize(allpass_size, 0.0);

        let clamped_feedback = feedback_amount.clamp(0.0, 1.0);
        let clamped_delay = delay_samples.min(buffer_capacity - 1);

        Self {
            input_buffer,
            output_buffer,
            allpass_buffer,
            sample_rate,
            delay_samples: clamped_delay,
            target_delay_samples: clamped_delay,
            feedback_amount: clamped_feedback,
            character_amount: 0.0,
            smoothing_samples: 0,
            current_smooth_step: 0,
            allpass_delay: allpass_size - 1,
            diffusion_amount: 0.65,
            lfo_phase: 0.0,
            lfo_frequency: 0.3,
            wow_flutter_depth: 2.5,
            noise_seed: 12345,
        }
    }

    fn generate_noise(&mut self) -> f32 {
        self.noise_seed = self.noise_seed.wrapping_mul(1103515245).wrapping_add(12345);
        let normalized = (self.noise_seed as f32) / (u32::MAX as f32);
        (normalized * 2.0) - 1.0
    }

    fn soft_clip(&self, input_sample: f32, amount: f32) -> f32 {
        if amount < 0.01 {
            return input_sample;
        }

        let threshold = 0.7;
        let ratio = 3.0 * amount;

        if input_sample.abs() < threshold {
            input_sample
        } else {
            let sign = input_sample.signum();
            let excess = input_sample.abs() - threshold;
            let clipped = threshold + excess / (1.0 + excess * ratio);
            input_sample * (1.0 - amount) + sign * clipped * amount
        }
    }

    fn tape_saturation(&self, input_sample: f32, amount: f32) -> f32 {
        if amount < 0.01 {
            return input_sample;
        }

        let drive = 1.0 + 0.2 * amount;
        let x = input_sample * drive;

        let saturated = if x.abs() < 0.5 {
            x * (1.0 + 0.2 * amount)
        } else {
            let sign = x.signum();
            sign * (0.5 + 0.5 * (1.0 - (-((x.abs() - 0.5) * 4.0)).exp()))
        };

        let saturated_scaled = saturated * 0.85;
        input_sample * (1.0 - amount) + saturated_scaled * amount
    }

    fn hermite_interpolate(&self, buffer: &VecDeque<f32>, delay_position: f32) -> f32 {
        let index = delay_position as usize;
        let fraction = delay_position - index as f32;

        let buffer_len = buffer.len();

        let sample_minus_1 = if index > 0 { buffer[index - 1] } else { 0.0 };
        let sample_0 = buffer[index];
        let sample_1 = buffer[(index + 1).min(buffer_len - 1)];
        let sample_2 = buffer[(index + 2).min(buffer_len - 1)];

        let c0 = sample_0;
        let c1 = 0.5 * (sample_1 - sample_minus_1);
        let c2 = sample_minus_1 - 2.5 * sample_0 + 2.0 * sample_1 - 0.5 * sample_2;
        let c3 = 0.5 * (sample_2 - sample_minus_1) + 1.5 * (sample_0 - sample_1);

        ((c3 * fraction + c2) * fraction + c1) * fraction + c0
    }

    fn allpass_filter(&mut self, input_sample: f32, amount: f32) -> f32 {
        if amount < 0.01 {
            return input_sample;
        }

        let delayed_sample = self.allpass_buffer[self.allpass_delay];
        let scaled_diffusion = self.diffusion_amount * amount;

        let output_sample = -input_sample * scaled_diffusion
            + delayed_sample
            + input_sample * scaled_diffusion * scaled_diffusion;

        self.allpass_buffer.rotate_right(1);
        self.allpass_buffer[0] = input_sample + delayed_sample * scaled_diffusion;

        input_sample * (1.0 - amount) + output_sample * amount
    }

    fn update_sample_rate(&mut self, new_sample_rate: usize) {
        let buffer_capacity = MAX_DELAY_SECONDS * new_sample_rate;
        self.input_buffer = Box::new(VecDeque::with_capacity(buffer_capacity));
        self.output_buffer = Box::new(VecDeque::with_capacity(buffer_capacity));
        self.input_buffer.resize(buffer_capacity, 0.0);
        self.output_buffer.resize(buffer_capacity, 0.0);

        let allpass_size = (new_sample_rate as f32 * 0.005) as usize;
        self.allpass_buffer = Box::new(VecDeque::with_capacity(allpass_size));
        self.allpass_buffer.resize(allpass_size, 0.0);
        self.allpass_delay = allpass_size - 1;

        self.sample_rate = new_sample_rate;
        self.delay_samples = self.delay_samples.min(buffer_capacity - 1);
        self.target_delay_samples = self.target_delay_samples.min(buffer_capacity - 1);
    }

    fn set_delay_samples(&mut self, delay_samples: usize) {
        let max_delay = MAX_DELAY_SECONDS * self.sample_rate - 1;
        self.target_delay_samples = delay_samples.min(max_delay);

        if self.target_delay_samples != self.delay_samples {
            self.smoothing_samples = (self.sample_rate as f32 * 0.022) as usize;
            self.current_smooth_step = 0;
        }
    }

    fn set_feedback(&mut self, feedback_amount: f32) {
        self.feedback_amount = feedback_amount.clamp(0.0, 1.0);
    }

    fn set_character(&mut self, character_amount: f32) {
        self.character_amount = character_amount.clamp(0.0, 1.0);
    }

    fn process_sample(&mut self, input_sample: f32) -> f32 {
        self.input_buffer.rotate_right(1);
        self.input_buffer[0] = input_sample;

        let current_delay = if self.current_smooth_step < self.smoothing_samples {
            let progress = self.current_smooth_step as f32 / self.smoothing_samples as f32;
            let smoothed_progress = progress * progress * (3.0 - 2.0 * progress);
            self.current_smooth_step += 1;

            let delay = self.delay_samples as f32
                + (self.target_delay_samples as f32 - self.delay_samples as f32)
                    * smoothed_progress;

            if self.current_smooth_step >= self.smoothing_samples {
                self.delay_samples = self.target_delay_samples;
            }

            delay
        } else {
            self.delay_samples as f32
        };

        let modulated_delay = if self.character_amount > 0.01 {
            let lfo_value = (self.lfo_phase * TWO_PI).sin();
            self.lfo_phase += self.lfo_frequency / self.sample_rate as f32;
            if self.lfo_phase >= 1.0 {
                self.lfo_phase -= 1.0;
            }

            let scaled_wow_flutter =
                self.wow_flutter_depth * self.character_amount * self.character_amount;
            let wow_flutter_samples = lfo_value * scaled_wow_flutter;
            (current_delay + wow_flutter_samples).max(0.0)
        } else {
            current_delay
        };

        let delayed_input = self.hermite_interpolate(&self.input_buffer, modulated_delay);
        let delayed_output = self.hermite_interpolate(&self.output_buffer, modulated_delay);

        let noise_amount = if self.character_amount > 0.01 {
            let noise = self.generate_noise();
            noise * 0.0015 * self.character_amount * self.character_amount
        } else {
            0.0
        };

        let delayed_with_noise = delayed_output + noise_amount;

        let saturated_feedback = self.tape_saturation(
            delayed_with_noise * self.feedback_amount,
            self.character_amount,
        );
        let feedback_signal = self.allpass_filter(saturated_feedback, self.character_amount);

        let output_sample = delayed_input + feedback_signal;
        let final_output = self.soft_clip(output_sample, self.character_amount);

        self.output_buffer.rotate_right(1);
        self.output_buffer[0] = final_output;

        final_output
    }

    fn reset(&mut self) {
        for sample in self.input_buffer.iter_mut() {
            *sample = 0.0;
        }
        for sample in self.output_buffer.iter_mut() {
            *sample = 0.0;
        }
        for sample in self.allpass_buffer.iter_mut() {
            *sample = 0.0;
        }
        self.lfo_phase = 0.0;
        self.noise_seed = 12345;
    }
}

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        type Delay;

        fn create_delay(
            sample_rate: usize,
            delay_samples: usize,
            feedback_amount: f32,
        ) -> Box<Delay>;
        fn update_sample_rate(self: &mut Delay, new_sample_rate: usize);
        fn set_delay_samples(self: &mut Delay, delay_samples: usize);
        fn set_feedback(self: &mut Delay, feedback_amount: f32);
        fn set_character(self: &mut Delay, character_amount: f32);
        fn process_sample(self: &mut Delay, input_sample: f32) -> f32;
        fn reset(self: &mut Delay);
    }
}

fn create_delay(sample_rate: usize, delay_samples: usize, feedback_amount: f32) -> Box<Delay> {
    Box::new(Delay::new(sample_rate, delay_samples, feedback_amount))
}
