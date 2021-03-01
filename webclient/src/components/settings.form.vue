<template>
  <div class="px-15">
    <md-subheader class="md-primary">Relay</md-subheader>
    <md-field :class="showError(on_valid)">
      <label>Turn-On Threshold</label>
      <md-input type="number" :value="s.on_threshold" @input="s.on_threshold = parseInt($event)"></md-input>
    </md-field>
    <md-field :class="showError(off_valid)">
      <label>Turn-Off Threshold</label>
      <md-input type="number" :value="s.off_threshold" @input="s.off_threshold = parseInt($event)"></md-input>
    </md-field>
    <md-field :class="showError(delay_valid)">
      <label>Turn-Off Delay</label>
      <md-input type="number" :value="s.off_delay" @input="s.off_delay = parseInt($event)"></md-input>
    </md-field>

    <md-divider/>
    <md-subheader class="md-primary">Wi-Fi</md-subheader>
    <md-field :class="showError(ssid_valid)">
      <label>SSID</label>
      <md-input v-model="s.wifi_ssid"></md-input>
    </md-field>
    <md-field :class="showError(pass_valid)">
      <label>Password (WPA2)</label>
      <md-input v-model="s.wifi_pass" type="password"></md-input>
    </md-field>
  </div>
</template>

<script>
import Vue from 'vue'

export default {
  name: 'SettingsForm',
  props: {
    value: {
      type: Object,
    }
  },
  data(){
    return {
      s: {}
    }
  },
  watch: {
    value: {
      handler(value){
        this.copyValue(value)
      },
      immediate: true
    },
    s: {
      handler(settings){
        this.$emit('input', settings)
      },
      deep: true
    },
    valid(valid){
      this.$emit('valid', valid)
    }
  },
  computed: {
    ssid_valid(){
      let value = this.s.wifi_ssid
      return typeof value == 'string' && value.length < 64
    },
    pass_valid(){
      let value = this.s.wifi_pass
      return typeof value == 'string' && value.length < 32
    },
    on_valid(){
      let value = this.s.on_threshold
      return typeof value == 'number' && value >= 0 && value <= 100
    },
    off_valid(){
      let value = this.s.off_threshold
      return typeof value == 'number' && value >= 0 && value <= this.s.on_threshold
    },
    delay_valid(){
      let value = this.s.off_threshold
      return typeof value == 'number' && value >= 0 && value <= 4294967295
    },
    valid(){
      return this.ssid_valid && this.pass_valid && this.on_valid && this.off_valid && this.delay_valid
    }
  },
  methods: {
    showError(isValid){
      return isValid ? '' : 'md-invalid'
    },
    copyValue(value){
      for(let key in value){
        Vue.set(this.s, key, value[key])
      }
    }
  }
}
</script>

<style scoped>

</style>
