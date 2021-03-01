<template>
  <div>

    <md-toolbar class="md-transparent" md-elevation="0">
      <md-button class="md-icon-button" @click="$emit('input', false)">
        <md-icon>arrow_back</md-icon>
      </md-button>
      <div class="md-title" style="flex: 1">Settings</div>
      <md-button class="md-icon-button" @click="getSettings">
        <md-icon>cached</md-icon>
      </md-button>
      <md-button class="md-icon-button" @click="saveSettings" :disabled="!valid">
        <md-icon :class="!valid ? 'text-disabled' : ''">save</md-icon>
      </md-button>
    </md-toolbar>

    <div class="px-15">
      <SettingsForm v-model="settings" @valid="valid = $event"/>
    </div>

    <md-snackbar :md-duration="4000" :md-active.sync="showSnackbar" md-persistent>
      <span>Settings successfully saved.</span>
    </md-snackbar>

  </div>
</template>

<script>
import SettingsForm from './settings.form'
import Service from '../service'

export default {
  name: 'Settings',
  components: {
      SettingsForm
  },
  props: {
    value: {
      type: Boolean,
    }
  },
  data(){
    return {
      settings: {
        wifi_ssid: '',
        wifi_pass: '',
        on_threshold: 0,
        off_threshold: 0,
        off_delay: 0
      },
      valid: true,
      showSnackbar: false
    }
  },
  created(){
    this.getSettings()
  },

  methods: {
    getSettings(){
      Service.getSettings().then((res) => {
        this.settings = res
      })
    },
    
    saveSettings(){
      Service.setSettings(this.settings).then((res) => {
        if(res == true){
          this.showSnackbar = true
        }
      })
    },
  }
}
</script>

<style scoped>

</style>
